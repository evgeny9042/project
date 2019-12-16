#pragma once

#include <QMainWindow>
#include <QWidget>
#include <QTimer>
#include <QLabel>

#include <queue>
#include <map>
#include <string>
#include <memory>

#include "./aiinterface.h"
#include "./game.h"

/// ������� ������ ��� ��������� ����
class MyPaintWidget : public QWidget
{
public:
  /// �����������
  MyPaintWidget(QWidget *parent, Game* g) : QWidget(parent), game(g) {}
  /// ���������� ����
  void paintEvent(QPaintEvent *) override;

private:
  /// ����
  Game *game;
};

/// ������� ����
class MainWindow: public QMainWindow
{
    Q_OBJECT

public:
  /// �����������
  MainWindow();
  /// ����������
  ~MainWindow();

protected: 
  /// ���������� �������� �������� ����
  void closeEvent(QCloseEvent *event) override;  
  /// ���������� ������� ������ ������� 
  /// ����� ��������� ����������, �������� ������ ����� � ����
  void keyPressEvent(QKeyEvent *key) override;

private slots:
  void on_up();
  void on_down();
  void on_timer();  
  void on_reset();
  void on_state_changed(bool);

private:
  /// ������� ������ �������� ����
  QWidget *m_central_widget{nullptr};
  /// ��� ��������� ����
  MyPaintWidget *m_paint_widget{nullptr};
  /// ��� ����������� ��������� �����
  QLabel *m_score_label; 

private:
  /// ����
  Game m_game; 

  /// ������� �������� ��
  std::shared_ptr<AI> m_current_ai{nullptr};

  std::map<std::string, std::shared_ptr<AI>> m_name2ai;
  std::shared_ptr<AI> get_ai(std::string name);

private:
  /// ���� ������ ����, �� ����� ������
  QTimer m_timer;
  /// ������ �������� ������������, ���� �� ������
  std::queue<int> m_actions;
};