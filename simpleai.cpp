
#include "./simpleai.h"

SimpleAi::SimpleAi(Game * game) : AI(game)
{
};

void SimpleAi::train()
{
}

void SimpleAi::play()
{
  if ( !m_game ) return;
  auto xy = m_game->ball_coords();
  auto y  = m_game->platform_position();
  if ( xy.second == y ) {
    m_game->step(0);
  } else if ( xy.second < y ) {
    m_game->step(1);
  } else {
    m_game->step(2);
  }
}