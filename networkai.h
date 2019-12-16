#pragma once

#include "./aiinterface.h"
#include "./game.h"

/// Deep reinforcement � �������������� ��������� �����
class NetworkAi : public AI
{
public:
  /// �����������
  NetworkAi(Game * game);
  /// �������������
  void train() override;
  /// ������� ��� � ����
  void play() override;

private:
  /// �������� � �������� ������� ���
  void play_and_record(int steps);
};
