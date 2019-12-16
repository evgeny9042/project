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

/// В впроцессе обучения будет иногда выбирать случайное действие, с выбранной
/// вероятностью. Причем вероятность выбрать случайное дейсвие будет снижаться
/// по мере обучения
//------------------------------------------------------------------------------
static float epsilon = 0.5f;
static void decrease_epsilon() { 
  epsilon *= 0.99f;
  if ( epsilon < 0.01 ) {
    epsilon = 0.01;
  }
}
/// Вспомогательные функции для преобразования из моего формата в формат dlib
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

/// Класс реализации записей игры (нужен для обучения нейронной сети)
/// Обучаться будем не на всем списке, а на рандомно выбранном подмножестве
/// Буфер реализован циклически, т.к. когда дошли до конца, начинаем с начала и 
/// перезаписываем старые игры
//------------------------------------------------------------------------------
template <int buf_size, int batch_size>
class ReplayBuffer
{
  /// одна запись 
  struct replay {
    Game::state state;      ///< состояние игры
    int action;             ///< предпринятое действие
    int reward;             ///< полученное вознаграждение
    Game::state next_state; ///< следующее состояние игры
    int done;               ///< признак завершения игры
  };

public:
  /// Конструктор
  ReplayBuffer() { replays.resize(buf_size); }  
  /// Добавить очередную запись
  void add(Game::state &s, int a, int r, Game::state next_s, int done) {
    if ( index >= buf_size ) index = 0;
    replays[index++] = replay{s, a, r, next_s, done};
  }
  /// Получить список записей
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
  /// получить параметры буфера и запись
  int bufsize()             const  { return buf_size;   }
  int batchsize()           const  { return batch_size; }
  const replay& get(int id) const  { return replays[id]; }

private:
  /// последняя позация
  int index{0};
  /// список всех записей
  std::vector<replay> replays; 
};

//------------------------------------------------------------------------------
/// Свой слой. Поскольку я так и не понял, какую функцию мне использовать для 
/// получения значения последнего слоя нейронноый сети, но до подсчета фукнции потерь,
/// я написал свой слой сетки, который служит исключительно для запоминания выхода сети
/// для одного примера
class mylayer_
{
public:
  /// Конструктор
  mylayer_() : m_qvalues(3,0) {}

  template <typename SUBNET>
  void setup(const SUBNET&) {}

  /// проход вперед по слою
  void forward_inplace(const tensor& input, tensor& /*output*/)
  {
    /// запоминаем значения Q фукнции
    tensor::const_iterator it = input.begin();
    m_qvalues[0] = *it;  it++;
    m_qvalues[1] = *it;  it++;
    m_qvalues[2] = *it;  it++;
  }

  /// без этих функций не компилировалось, они мне не нужны (все 5)
  void backward_inplace(const tensor& /*gradient_input*/, tensor& /*data_grad*/, tensor& /*params_grad*/) {}
  const tensor& get_layer_params() const { return params; }
  tensor& get_layer_params()             { return params; }
  friend void serialize(const mylayer_&, std::ostream&) {}
  friend void deserialize(mylayer_&, std::istream&)     {} 

public:
  /// получить список значений Q функции
  const std::vector<float>& get_qvalues() { return m_qvalues; }
  /// получить конкретное значение Q функции 
  float get_qvalue(int index)             { return m_qvalues[index]; }
  /// получить значение V функции
  float get_value()                       { return *std::max_element(m_qvalues.begin(), m_qvalues.end()); }

  /// Получить действие, которое надо совершить в игре
  int get_best_action() 
  {    
    int best_action = std::distance(m_qvalues.begin(), std::max_element(m_qvalues.begin(), m_qvalues.end()));
    assert(best_action >= 0); assert(best_action <= 2);
    return best_action;
  }
  /// Получить действие, которое надо совершить в игре, или рандомное, если тренируемся
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
  /// Список значений фукнции Q(state, action) - выход нейронной сети
  std::vector<float> m_qvalues;

private:
  /// Ненужные мне параметры, которые создал, чтобы тока компилировалось
  resizable_tensor params;
};

//------------------------------------------------------------------------------
/// Конфирурация сетки
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

static net_type net_agent;    ///< сетка для обучения 
static net_type net_target;   ///< сетка только для тренировки

static const int buffer_size = 10000;
static const int batch_size  = 500;
static ReplayBuffer<buffer_size, batch_size> buffer; // буфер для записи процесса игр

static const float discount{0.99f}; ///< для вычисления дисконтированного вознаграждения

/// оценить, насколько хорошо играем в данный момент
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

/// Програть обучение и вычслить значение loss функции
static void compute_td_loss()
{
  std::vector<matrix<float>> X, Y;
  X.reserve(batch_size); Y.reserve(batch_size);  
  
  for ( int idx : buffer.sample() ) {
    const auto& repl = buffer.get(idx);
    matrix<float> state = translate(repl.state);
    // прогоняем через обучающуюся сеть и получаем текущее значение Q фукнции для 3 различных действий
    net_agent(state); 
    const std::vector<float> &predicted_qvalues = net_agent.subnet().layer_details().get_qvalues();

    // оцениванием то, что сейчас есть по необучаемой сети
    net_target(translate(repl.next_state)); 
    // вычисляем значение, которое должно быть для выбранного ранее действия
    float target_qvalue = repl.reward + discount * net_target.subnet().layer_details().get_value() * repl.done;

    X.push_back(state);   
    Y.push_back(translate(predicted_qvalues, repl.action, target_qvalue));
  }

  // тренируем
  dnn_trainer<net_type, adam> trainer(net_agent, adam(0.0005, 0.9, 0.999));
  trainer.set_max_num_epochs(1);
  trainer.set_learning_rate(0.001);
  trainer.set_mini_batch_size(batch_size);
  trainer.train(X, Y);
}

//------------------------------------------------------------------------------
NetworkAi::NetworkAi(Game * game) : AI(game)
{ 
  // если есть предобученная модель, грузим ее
  // если ее нет, то надо вызвать функцию train
  // Однако тренировка может занять порядка 4 дней
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
  // заполянем буфер по полной
  play_and_record(buffer_size); 

  // количество итераций тренировки (от болды взял, таки чему-то научиться длжен)
  const int number = 130000;                              
  for ( int i = 0; i < number; i++ ) {
    compute_td_loss();       // тренируем сеть
    play_and_record(1000);   // перезаписываем новыми значениями 

    if ( i % 10 == 0 ) {
      out << evaluate(m_game) << "\n";
      out.flush();
    }
    // уменьшаем вероятность выбрать случайное действие
    if ( i % 500 == 0 ) { 
      decrease_epsilon(); 
    }
    // время от времени копируем сеть, которую обучаем в сеть с помощью которой 
    // оцениваем значение Q функции 
    if ( i % 500 == 0 ) { 
      net_target = net_agent;
    }
  }
  /// сохраняем полученную сеть на диск, так как процесс тренировки может занять
  /// очень много времени и не хочется его потом повторять
  serialize("net_agent.dnn") << net_agent;
}

void NetworkAi::play()
{
  if ( !m_game ) return;
  matrix<float> state = translate(m_game->observation());
  // получаем значение Q функции для 3 действий
  net_agent(state); 
  // на основе значение Q функции получаем лучшее дейтсвие
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


