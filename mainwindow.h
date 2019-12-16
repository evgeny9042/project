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

/// Главный виджет для отрисовки игры
class MyPaintWidget : public QWidget
{
public:
  /// Конструктор
  MyPaintWidget(QWidget *parent, Game* g) : QWidget(parent), game(g) {}
  /// Отрисовать игру
  void paintEvent(QPaintEvent *) override;

private:
  /// Игра
  Game *game;
};

/// Главное окно
class MainWindow: public QMainWindow
{
    Q_OBJECT

public:
  /// Конструктор
  MainWindow();
  /// Деструктор
  ~MainWindow();

protected: 
  /// Обработать закрытие главного окна
  void closeEvent(QCloseEvent *event) override;  
  /// Обработать нажатик кнопок игроком 
  /// можно управлять платформой, нажатием клавиш Вверх и Вниз
  void keyPressEvent(QKeyEvent *key) override;

private slots:
  void on_up();
  void on_down();
  void on_timer();  
  void on_reset();
  void on_state_changed(bool);

private:
  /// Главный виджет главного окна
  QWidget *m_central_widget{nullptr};
  /// для отрисовки игры
  MyPaintWidget *m_paint_widget{nullptr};
  /// для отображения набранных очков
  QLabel *m_score_label; 

private:
  /// Игра
  Game m_game; 

  /// Текущий играющий ИИ
  std::shared_ptr<AI> m_current_ai{nullptr};

  std::map<std::string, std::shared_ptr<AI>> m_name2ai;
  std::shared_ptr<AI> get_ai(std::string name);

private:
  /// Если играем сами, то нужен таймер
  QTimer m_timer;
  /// Список действий пользователя, если он играет
  std::queue<int> m_actions;
};