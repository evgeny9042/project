#pragma once

#include "./aiinterface.h"
#include "./game.h"

/// Deep reinforcement с использованием нейронных сетей
class NetworkAi : public AI
{
public:
  /// Конструктор
  NetworkAi(Game * game);
  /// Натренировать
  void train() override;
  /// Сделать шаг в игре
  void play() override;

private:
  /// поиграть и записать процесс игр
  void play_and_record(int steps);
};
