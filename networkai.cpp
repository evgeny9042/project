#include "./networkai.h"

#include <algorithm>
#include <random> 

#include <QFile> 
#include <QTextStream> 
#include <QString> 
#include <QStringList> 

#include "C:/otus/dlib-19.18/dlib/dnn.h"
#include "C:/otus/dlib-19.18/dlib/data_io.h"
#include "C:/otus/dlib-19.18/dlib/cuda/tensor.h"

using namespace dlib;

/// � ��������� �������� ����� ������ �������� ��������� ��������, � ���������
/// ������������. ������ ����������� ������� ��������� ������� ����� ���������
/// �� ���� ��������
//------------------------------------------------------------------------------
static float epsilon = 0.5f;
static void decrease_epsilon() { 
  epsilon *= 0.99f;
  if ( epsilon < 0.01 ) {
    epsilon = 0.01;
  }
}
/// ��������������� ������� ��� �������������� �� ����� ������� � ������ dlib
//------------------------------------------------------------------------------
static matrix<float> translate(Game::state s)
{
  matrix<float> state;
  std::vector<float>  v(5, 0);
  state.set_size(5, 1);
  state(0, 0) = v[0] = std::get<0>(s) / 10.0;
  state(1, 0) = v[1] = std::get<1>(s) / 100.0;
  state(2, 0) = v[2] = std::get<2>(s) / 100.0;
  state(3, 0) = v[3] = std::get<3>(s) / 100.0;
  state(4, 0) = v[4] = std::get<4>(s) / 100.0;
  return state;
}

static matrix<float> translate(const std::vector<float> &qvalues, 
  int action, float pred_qvalue)
{
  matrix<float> ret;
  assert(qvalues.size() == 3);
  ret.set_size(3, 1);
  ret(0, 0) = qvalues[0];
  ret(1, 0) = qvalues[1];
  ret(2, 0) = qvalues[2];

  ret(action, 0) = pred_qvalue;
  return ret;
}

/// ����� ���������� ������� ���� (����� ��� �������� ��������� ����)
/// ��������� ����� �� �� ���� ������, � �� �������� ��������� ������������
/// ����� ���������� ����������, �.�. ����� ����� �� �����, �������� � ������ � 
/// �������������� ������ ����
//------------------------------------------------------------------------------
template <int buf_size, int batch_size>
class ReplayBuffer
{
  /// ���� ������ 
  struct replay {
    Game::state state;      ///< ��������� ����
    int action;             ///< ������������ ��������
    int reward;             ///< ���������� ��������������
    Game::state next_state; ///< ��������� ��������� ����
    int done;               ///< ������� ���������� ����
  };

public:
  /// �����������
  ReplayBuffer() { replays.resize(buf_size); }  
  /// �������� ��������� ������
  void add(Game::state &s, int a, int r, Game::state next_s, int done) {
    if ( index >= buf_size ) index = 0;
    replays[index++] = replay{s, a, r, next_s, done};
  }
  /// �������� ������ �������
  std::vector<int> sample() {
    assert(buf_size >= batch_size);

    static std::random_device rd;   static std::mt19937 re(rd());    
    static std::uniform_int_distribution<> dis(0, buf_size - 1);
    std::vector<int> to_return(batch_size);
    std::generate(to_return.begin(), to_return.end(), [&] () {
      int it = dis(re);  assert(it >= 0); assert(it < buf_size);
      return it;
    });
    return to_return;
  }
  /// �������� ��������� ������ � ������
  int bufsize()             const  { return buf_size;   }
  int batchsize()           const  { return batch_size; }
  const replay& get(int id) const  { return replays[id]; }

private:
  /// ��������� �������
  int index{0};
  /// ������ ���� �������
  std::vector<replay> replays; 
};

//------------------------------------------------------------------------------
/// ���� ����. ��������� � ��� � �� �����, ����� ������� ��� ������������ ��� 
/// ��������� �������� ���������� ���� ���������� ����, �� �� �������� ������� ������,
/// � ������� ���� ���� �����, ������� ������ ������������� ��� ����������� ������ ����
/// ��� ������ �������
class mylayer_
{
public:
  /// �����������
  mylayer_() : m_qvalues(3,0) {}

  template <typename SUBNET>
  void setup(const SUBNET&) {}

  /// ������ ������ �� ����
  void forward_inplace(const tensor& input, tensor& /*output*/)
  {
    /// ���������� �������� Q �������
    tensor::const_iterator it = input.begin();
    m_qvalues[0] = *it;  it++;
    m_qvalues[1] = *it;  it++;
    m_qvalues[2] = *it;  it++;
  }

  /// ��� ���� ������� �� ���������������, ��� ��� �� ����� (��� 5)
  void backward_inplace(const tensor& /*gradient_input*/, tensor& /*data_grad*/, tensor& /*params_grad*/) {}
  const tensor& get_layer_params() const { return params; }
  tensor& get_layer_params()             { return params; }
  friend void serialize(const mylayer_&, std::ostream&) {}
  friend void deserialize(mylayer_&, std::istream&)     {} 

public:
  /// �������� ������ �������� Q �������
  const std::vector<float>& get_qvalues() { return m_qvalues; }
  /// �������� ���������� �������� Q ������� 
  float get_qvalue(int index)             { return m_qvalues[index]; }
  /// �������� �������� V �������
  float get_value()                       { return *std::max_element(m_qvalues.begin(), m_qvalues.end()); }

  /// �������� ��������, ������� ���� ��������� � ����
  int get_best_action() 
  {    
    int best_action = std::distance(m_qvalues.begin(), std::max_element(m_qvalues.begin(), m_qvalues.end()));
    assert(best_action >= 0); assert(best_action <= 2);
    return best_action;
  }
  /// �������� ��������, ������� ���� ��������� � ����, ��� ���������, ���� �����������
  int get_action(bool random) 
  {
    if ( !random ) {
      return get_best_action();
    }
    static std::random_device rd;
    static std::mt19937 re(rd());
    static std::uniform_real_distribution<> dis(0, 1); // rage 0 - 1
    static std::uniform_int_distribution<> dis_action(0, 2);

    int chosen_action = 0;
    if ( dis(re) <= epsilon ) {
      chosen_action = dis_action(re);
      assert(chosen_action >= 0); assert(chosen_action <= 2);
    } else {
      chosen_action = get_best_action();
    }
    return chosen_action;
  }  

private:
  /// ������ �������� ������� Q(state, action) - ����� ��������� ����
  std::vector<float> m_qvalues;

private:
  /// �������� ��� ���������, ������� ������, ����� ���� ���������������
  resizable_tensor params;
};

//------------------------------------------------------------------------------
/// ������������ �����
template <typename SUBNET>
using mylayer  = add_layer<mylayer_, SUBNET>;
using net_type = loss_mean_squared_multioutput<
  mylayer<
  fc<3,
  relu<fc<32,
  relu<fc<64,
  relu<fc<128,
  relu<fc<128,
  input<matrix<float>>
  >>>>>>>>>>>;

static net_type net_agent;    ///< ����� ��� �������� 
static net_type net_target;   ///< ����� ������ ��� ����������

static const int buffer_size = 10000;
static const int batch_size  = 500;
static ReplayBuffer<buffer_size, batch_size> buffer; // ����� ��� ������ �������� ���

static const float discount{0.99f}; ///< ��� ���������� ����������������� ��������������

/// �������, ��������� ������ ������ � ������ ������
static Game::score_t evaluate(Game * game)
{
  game->reset();
  while ( !game->done() ) {    
    net_agent(translate(game->observation()));
    int a = net_agent.subnet().layer_details().get_action(false); 
    game->step(a);
  }
  Game::score_t score = game->score();
  game->reset();
  return score;
}

/// �������� �������� � �������� �������� loss �������
static void compute_td_loss()
{
  std::vector<matrix<float>> X, Y;
  X.reserve(batch_size); Y.reserve(batch_size);  
  
  for ( int idx : buffer.sample() ) {
    const auto& repl = buffer.get(idx);
    matrix<float> state = translate(repl.state);
    // ��������� ����� ����������� ���� � �������� ������� �������� Q ������� ��� 3 ��������� ��������
    net_agent(state); 
    const std::vector<float> &predicted_qvalues = net_agent.subnet().layer_details().get_qvalues();

    // ����������� ��, ��� ������ ���� �� ����������� ����
    net_target(translate(repl.next_state)); 
    // ��������� ��������, ������� ������ ���� ��� ���������� ����� ��������
    float target_qvalue = repl.reward + discount * net_target.subnet().layer_details().get_value() * repl.done;

    X.push_back(state);   
    Y.push_back(translate(predicted_qvalues, repl.action, target_qvalue));
  }

  // ���������
  dnn_trainer<net_type, adam> trainer(net_agent, adam(0.0005, 0.9, 0.999));
  trainer.set_max_num_epochs(1);
  trainer.set_learning_rate(0.001);
  trainer.set_mini_batch_size(batch_size);
  trainer.train(X, Y);
}

//------------------------------------------------------------------------------
NetworkAi::NetworkAi(Game * game) : AI(game)
{ 
  // ���� ���� ������������� ������, ������ ��
  // ���� �� ���, �� ���� ������� ������� train
  // ������ ���������� ����� ������ ������� 4 ����
  try {
    deserialize("net_agent.dnn") >> net_agent;
  } catch ( const std::exception& ) {
  }  
}

void NetworkAi::train()
{
  epsilon = 0.5f;
  QFile file("scores_networkai.txt");
  if ( !file.open(QIODevice::WriteOnly | QIODevice::Text) )
    return;
  QTextStream out(&file);

  net_target = net_agent;

  m_game->reset();
  // ��������� ����� �� ������
  play_and_record(buffer_size); 

  // ���������� �������� ���������� (�� ����� ����, ���� ����-�� ��������� �����)
  const int number = 130000;                              
  for ( int i = 0; i < number; i++ ) {
    compute_td_loss();       // ��������� ����
    play_and_record(1000);   // �������������� ������ ���������� 

    if ( i % 10 == 0 ) {
      out << evaluate(m_game) << "\n";
      out.flush();
    }
    // ��������� ����������� ������� ��������� ��������
    if ( i % 500 == 0 ) { 
      decrease_epsilon(); 
    }
    // ����� �� ������� �������� ����, ������� ������� � ���� � ������� ������� 
    // ��������� �������� Q ������� 
    if ( i % 500 == 0 ) { 
      net_target = net_agent;
    }
  }
  /// ��������� ���������� ���� �� ����, ��� ��� ������� ���������� ����� ������
  /// ����� ����� ������� � �� ������� ��� ����� ���������
  serialize("net_agent.dnn") << net_agent;
}

void NetworkAi::play()
{
  if ( !m_game ) return;
  matrix<float> state = translate(m_game->observation());
  // �������� �������� Q ������� ��� 3 ��������
  net_agent(state); 
  // �� ������ �������� Q ������� �������� ������ ��������
  int a = net_agent.subnet().layer_details().get_action(false);
  m_game->step(a);
}

void NetworkAi::play_and_record(int steps)
{  
  for ( int i = 0; i < steps; i++ ) {
    Game::state s = m_game->observation();
    net_agent(translate(s));
    auto & layer = net_agent.subnet().layer_details();
    int a = layer.get_action(true);
    m_game->step(a);
    int is_done = m_game->done() ? 0 : 1;
    int reward  = m_game->done() ? -1 : 1;
    
    Game::state next_s = m_game->observation();
    buffer.add(s, a, reward, next_s, is_done);
    if ( m_game->done() ) {
      m_game->reset();
    }
  }
}


