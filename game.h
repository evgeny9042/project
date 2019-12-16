#pragma once

#include <utility>
#include <tuple> 

/// Класс, реализующий игру
class Game
{
public:  
  using score_t = int;  
  /// Текущее стостояние игры определяется следующими параметрами: 
  /// Модуль скорость мяча, направление движения мяча,
  /// координаты мяча по оси X,Y и коордитана Y платформы
  using state = std::tuple<int, int, int, int, int>;

public:
  /// Конструктор
  Game();
  /// Начать заново
  void reset();
  /// 0 - стоим на месте
  /// 1 - двигаем площадку вверх
  /// 2 - двигаем площадку вниз
  void step(int action);
  /// получить текущее состояние игры
  state observation();
  /// закончилась ли игра
  bool done() const;   
  /// получить 
  score_t score() const;  

public:
  /// вспомогательные функции для отрисовки игры
  int left()   const { return m_border_x.first;  };
  int right()  const { return m_border_x.second; };
  int top()    const { return m_border_y.first;  };
  int bottom() const { return m_border_y.second; };

  int ball_radius()                 const { return m_ball_radius; }
  std::pair<int, int> ball_coords() const { return m_ball_coords; }

  int platform_position() const { return m_platform_y;    };
  int platform_size()     const { return m_platform_size; }

private:
  /// тип координат
  using coord_t = int;
  using coords_t = std::pair<coord_t, coord_t>;

private:
  bool m_done{false};            ///< Признак окончания игры

private:
  const coords_t m_border_x;     ///< граница по оси X
  const coords_t m_border_y;     ///< граница по оси Y

private:  
  int m_velocity;                ///< скорость мяча  
  int m_course;                  ///< курс мяча
  const int m_ball_radius{6};    ///< радиус мяча  
  std::pair<coord_t, coord_t> m_ball_coords; ///< координаты мяча

private:  
  coord_t m_platform_y;          ///< координата центра площадки по оси Y
  const int m_platform_size{20}; ///< размер площадки (вверх и вниз от центра)
  const int m_platform_step{10}; ///< шаг площадки по оси Y

private:
  const int m_min_velocity{10};  ///< минимальная скорость мяча
  const int m_max_velocity{80};  ///< максимальная скорость мяча
  const int m_step_velocity{2};  ///< шаг

private:  
  score_t m_score; ///< текущее значение очков, набранное игроком
  const score_t m_max_score{1000}; ///< максимально возможный выигрыш
  void increase_score();
};