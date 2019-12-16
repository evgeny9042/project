#pragma once

#include <map>
#include <string>
#include <utility>

#include "./aiinterface.h"
#include "./game.h"


/// Реализация классического reinforcement learning
/// В качестве состояния выступает 
class ReinforceAi : public AI
{
public:
  ReinforceAi(Game * game);
  
  /// Натренировать
  void train() override;
  /// Сделать шаг в игре
  void play() override;

private:
  Game::score_t play_and_train();  
  void update(const Game::state &s, int a, int r, const Game::state &next_s, int is_done);

  void save_qvalues(std::string file_name) const;
  void load_qvalues(std::string file_name);

private:
  using value_t = float;

  int get_action(const Game::state &s, bool random);
  int get_best_action(const Game::state &s);

  value_t get_value(const Game::state &s);
  value_t get_qvalue(const Game::state &s, int a);

  void set_qvalue(const Game::state &s, int a, value_t qvalue);

private:  
  std::map<std::pair<Game::state, int>, value_t> qvalues;

private:
  float m_epsilon;
  const float m_discount{0.99f};
  const float m_alpha{0.5f};
};