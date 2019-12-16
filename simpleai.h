#pragma once

#include "./aiinterface.h"
#include "./game.h"

/// Класс, реализующий самый простой ИИ
/// Площадку двигаем туда, где в данный момент находится шарик
class SimpleAi : public AI
{
public:
  /// Конструктор
  SimpleAi(Game * game);
  /// Натренировать
  void train() override;
  /// Сделать шаг в игре
  void play() override;
};