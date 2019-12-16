
#include <random>  
#include <algorithm>
#include <cmath>

#include "./game.h"

Game::Game() : m_border_x(10, 500), m_border_y(10, 300)
{
  reset();
}

void Game::reset()
{
  m_score = 0;
  m_done  = false;

  m_platform_y = static_cast<int>(m_border_y.first + (m_border_y.second - m_border_y.first) / 2.0);
  // генерируем координаты м€ча возле правой стены на рандомной высоте
  static std::random_device rd;
  static std::mt19937 re(rd());
  static std::uniform_int_distribution<coord_t> dis_y(m_border_y.first + m_ball_radius,
    m_border_y.second - m_ball_radius);
  m_ball_coords = std::make_pair(m_border_x.second - m_ball_radius, dis_y(re));

  // генерурем скорость и курс
  static std::uniform_int_distribution<int> dis_course(135, 225);
  m_course = dis_course(re);
  
  m_velocity = m_min_velocity;
}

void Game::step(int action)
{
  if ( m_done ) return;
  // перемещаем площадку
  switch ( action ) {
  case 0: // стоим на месте
    break;
  case 1: // вверх  
    m_platform_y -= m_platform_step;
    if ( m_platform_y - m_platform_size < m_border_y.first ) {
      m_platform_y = m_border_y.first + m_platform_size;
    }    
    break;
  case 2: // вниз    
    m_platform_y += m_platform_step;
    if ( m_platform_y + m_platform_size > m_border_y.second ) {
      m_platform_y = m_border_y.second - m_platform_size;
    }
    break;
  default:
    break;
  }

  double PI = 3.14159265;

  // определ€ем новые теоретические координаты м€ча
  auto cos_ = cos(m_course * PI / 180);
  auto sin_ = sin(m_course * PI / 180);
  coord_t delta_x = static_cast<int>(cos_ * m_velocity);
  coord_t delta_y = static_cast<int>(-1 * (sin_ * m_velocity));
  m_ball_coords.first += delta_x;
  m_ball_coords.second += delta_y;

  // удар о верх или низ
  int down_y = m_ball_coords.second + m_ball_radius;
  int up_y = m_ball_coords.second - m_ball_radius;

  if ( up_y < m_border_y.first ) {
    m_ball_coords.second += m_border_y.first - up_y;
    m_course = 360 - m_course;
  }

  if ( down_y > m_border_y.second ) {
    m_ball_coords.second -= down_y - m_border_y.second;
    m_course = 360 - m_course;
  }

  // удар о право
  int right_x = m_ball_coords.first + m_ball_radius;
  if ( right_x > m_border_x.second ) {
    m_ball_coords.first -= right_x - m_border_x.second;
    if ( m_course <= 180 )
      m_course = 180 - m_course;
    else {
      m_course = m_course - 180;
      m_course = 360 - m_course;
    }
  }


  // удар о лево (о платформу)
  int left_x = m_ball_coords.first - m_ball_radius;
  if ( left_x < m_border_x.first ) {
    if ( abs(m_ball_coords.second - m_platform_y) <= m_platform_size ) {
      // попали на платформу
      m_ball_coords.first += m_border_x.first - left_x;

      if ( m_course <= 180 ) { // если летели вверх        
        if ( action == 2 ) { // а платформа сдвинулась вниз
          m_course = 180 + m_course;
          m_velocity = std::max(m_velocity - m_step_velocity, m_min_velocity);
        } else {
          m_course = 180 - m_course;
          if (action == 1)
            m_velocity = std::min(m_velocity + m_step_velocity, m_max_velocity);
        }
      }
      else {  // если летели вниз
        if ( action == 1 ) { // а платформа сдвинулась вверх
          m_course = m_course - 180;
          m_velocity = std::max(m_velocity - m_step_velocity, m_min_velocity);
        } else {          
          m_course = m_course - 180;
          m_course = 360 - m_course;
          if ( action == 2 )
            m_velocity = std::min(m_velocity + m_step_velocity, m_max_velocity);
        }
      }


    } else {
      // пролетели мимо платформы
      m_done = true;
    }
  }
  // ѕока играем, очки увеличиваютс€
  increase_score();
}

void Game::increase_score()
{
  if ( m_score != m_max_score ) {
    m_score++;
  } else {
    m_done = true;
  }
}

Game::state Game::observation()
{
  return std::make_tuple(m_velocity, m_course, m_ball_coords.first, 
    m_ball_coords.second, m_platform_y);
}

bool Game::done() const 
{ return m_done; };

Game::score_t Game::score() const
{
  return m_score;
}





//template <typename out_t>
//void render(out_t& out)
//{
//  out << "Course = " << m_course << "\n";
//  coord_t x_step = 1;
//  coord_t y_step = 1;
//  for ( coord_t y = m_border_y.first; y <= m_border_y.second; y += y_step ) {
//    for ( coord_t x = m_border_x.first; x <= m_border_x.second; x += x_step ) {
//      if ( y == m_border_y.first || y == m_border_y.second ) {
//        out << "-";
//      } else if ( x == m_border_x.second ) {
//        out << "|";
//      } else if ( x == m_ball_coords.first && y == m_ball_coords.second ) {
//        out << "0";
//      } else if ( std::abs(y - m_platform_y) <= m_platform_size  && x == m_border_x.first ) {
//        out << "|";
//      } else {
//        out << " ";
//      }
//    }
//    out << "\n";
//  }
//}