#pragma once

#include "./aiinterface.h"
#include "./game.h"

/// �����, ����������� ����� ������� ��
/// �������� ������� ����, ��� � ������ ������ ��������� �����
class SimpleAi : public AI
{
public:
  /// �����������
  SimpleAi(Game * game);
  /// �������������
  void train() override;
  /// ������� ��� � ����
  void play() override;
};