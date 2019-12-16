#include "./reinforceai.h"

#include <algorithm>
#include <random> 
#include <cassert> 
#include <iostream> 

#include <QFile> 
#include <QTextStream> 
#include <QString> 
#include <QStringList> 

static Game::state transform(const Game::state &s)
{
  int v, c, bx, by, py;
  std::tie(v, c, bx, by, py) = s;
  v  = int(floor(v / 10.0 + 0.5) * 10);
  c  = int(floor(c / 10.0 + 0.5) * 10);
  bx = int(floor(bx / 10.0 + 0.5) * 10);
  by = int(floor(by / 10.0 + 0.5) * 10);
  py = int(floor(py / 10.0 + 0.5) * 10);
  return std::make_tuple(v, c, bx, by, py);
}

ReinforceAi::ReinforceAi(Game * game) : AI(game)
{ 
  // если есть предобученная модель, грузим ее
  // если ее нет, то надо вызвать функцию train
  // Однако тренировка может занять порядка 2 дней
 
  load_qvalues("qvalues.txt");
};

void ReinforceAi::train()
{
  m_epsilon = 0.3f;
  QFile file("scores_reinforceai.txt");
  if ( !file.open(QIODevice::WriteOnly | QIODevice::Text) ) 
    return;
  QTextStream out(&file);

  const int number = 10000000;
  for ( int i = 0; i < number; i++ ) {
    Game::score_t score = play_and_train();
    if ( i % 10 == 0 ) {
      out << score << "\n";
    }    
    if ( i % 20000 == 0 ) {
      m_epsilon *= 0.99f;
    }
  }
  save_qvalues("qvalues.txt");
}

void ReinforceAi::play()
{
  Game::state s = transform(m_game->observation());
  int a = get_action(s, false);
  m_game->step(a);
}

void ReinforceAi::save_qvalues(std::string file_name) const
{
  QFile file(QString::fromStdString(file_name));
  if ( !file.open(QIODevice::WriteOnly | QIODevice::Text) ) return;
  QTextStream out(&file);

  int v, c, bx, by, py, a;
  for ( auto it = qvalues.cbegin(); it != qvalues.end(); it++ ) {    
    std::tie(v, c, bx, by, py) = it->first.first;
    a = it->first.second;
    out << v << " " << c << " " << bx << " " << by << " " 
      << py << " " << a << " " << it->second <<  "\n";
  }
}

void ReinforceAi::load_qvalues(std::string file_name)
{
  QFile file(QString::fromStdString(file_name));
  if ( !file.open(QIODevice::ReadOnly | QIODevice::Text) ) 
    return;
  QTextStream in(&file);
  while ( !in.atEnd() ) {
    QString line = in.readLine();
    QStringList list = line.split(' ');
    assert(list.size() == 7);
    Game::state s = std::make_tuple(list[0].toInt(), list[1].toInt(), 
      list[2].toInt(), list[3].toInt(), list[4].toInt());
    int action = list[5].toInt();
    value_t qvalue = list[6].toFloat();
    set_qvalue(s, action, qvalue);
  }
}


Game::score_t ReinforceAi::play_and_train()
{
  m_game->reset();
  Game::state s = transform(m_game->observation());
  while ( !m_game->done() ) {
    int a = get_action(s, true);
    m_game->step(a);
    Game::state next_s = transform(m_game->observation());
    int is_done = m_game->done() ? 0 : 1;
    int reward = 1;
    if ( is_done == 0 )
      reward = -reward;
    update(s, a, reward, next_s, is_done);
    s = next_s;
  }
  return m_game->score();
}

ReinforceAi::value_t ReinforceAi::get_qvalue(const Game::state &state, int action)
{
  auto it = qvalues.find(std::make_pair(state, action));
  if ( it == qvalues.end()) {
    return 0;
  }
  return it->second;
}

void ReinforceAi::set_qvalue(const Game::state &state, int action, value_t qvalue)
{
  qvalues[std::make_pair(state, action)] = qvalue;
}

ReinforceAi::value_t ReinforceAi::get_value(const Game::state &state)
{
  ReinforceAi::value_t values[3];
  for ( int action = 0; action < 3; action++ ) {
    values[action] = get_qvalue(state, action);
  }
  return *std::max_element(values, values + 3);
}

int ReinforceAi::get_action(const Game::state &state, bool random)
{
  if ( !random ) {
    return get_best_action(state);
  }
  static std::random_device rd;
  static std::mt19937 re(rd());
  static std::uniform_real_distribution<> dis(0, 1); // rage 0 - 1
  static std::uniform_int_distribution<> dis_action(0, 2);

  int chosen_action = 0;
  if ( dis(re) <= m_epsilon ) {
    chosen_action = dis_action(re);
    assert(chosen_action >= 0);
    assert(chosen_action <= 2);
  } else {
    chosen_action = get_best_action(state);
  }
  return chosen_action;
}

int ReinforceAi::get_best_action(const Game::state &state)
{
  ReinforceAi::value_t values[3];
  for ( int action = 0; action < 3; action++ ) {
    values[action] = get_qvalue(state, action);
  }
  int best_action = std::distance(values, std::max_element(values, values + 3));
  assert(best_action >= 0);
  assert(best_action <= 2);
  return best_action;
}

void ReinforceAi::update(const Game::state &state, int action, int reward,
  const Game::state &next_state, int is_done)
{
  assert(is_done >= 0);
  assert(is_done <= 1);
  value_t qvalue = (1 - m_alpha) * get_qvalue(state, action) + 
    m_alpha*(reward + m_discount * get_value(next_state) * is_done);
  set_qvalue(state, action, qvalue);
}