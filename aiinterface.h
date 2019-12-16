#pragma once

class Game;

/// Интерфейс ИИ 
struct AI
{
  /// Конструктор
  AI(Game *game) : m_game(game) {}
  /// Деструктор
  virtual ~AI() = default;
  
  /// Натренировать
  virtual void train() = 0;
  /// Играть
  virtual void play()  = 0;

protected:
  /// Игра
  Game * m_game{nullptr};
};