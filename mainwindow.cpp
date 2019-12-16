
#include <QPainter>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QKeyEvent>
#include <QString>
#include <QGroupBox>
#include <QRadioButton>

#include <cassert>

#include "./mainwindow.h"
#include "./simpleai.h"
#include "./reinforceai.h"
#include "./networkai.h"

MainWindow::MainWindow() 
{
  connect(&m_timer, SIGNAL(timeout()), this, SLOT(on_timer()));
  m_timer.setInterval(50);
  m_central_widget = new QWidget(this);
  this->setCentralWidget(m_central_widget);
  
  m_paint_widget = new MyPaintWidget(m_central_widget, &m_game);

  QRadioButton *check_box_user         = new QRadioButton("user",      this);
  check_box_user->setChecked(true);
  QRadioButton *check_box_simple_ai    = new QRadioButton("simple",    this);
  QRadioButton *check_box_network_ai   = new QRadioButton("reinforce", this);
  QRadioButton *check_box_reinforce_ai = new QRadioButton("network",   this);

  connect(check_box_user,         SIGNAL(clicked(bool)), this, SLOT(on_state_changed(bool)));
  connect(check_box_simple_ai,    SIGNAL(clicked(bool)), this, SLOT(on_state_changed(bool))); 
  connect(check_box_network_ai,   SIGNAL(clicked(bool)), this, SLOT(on_state_changed(bool)));
  connect(check_box_reinforce_ai, SIGNAL(clicked(bool)), this, SLOT(on_state_changed(bool)));  

  QVBoxLayout *group_box_layout = new QVBoxLayout();
  group_box_layout->addWidget(check_box_user);
  group_box_layout->addWidget(check_box_simple_ai);
  group_box_layout->addWidget(check_box_network_ai);
  group_box_layout->addWidget(check_box_reinforce_ai);  
  
  QGroupBox *groupBox = new QGroupBox("AI", this);
  groupBox->setLayout(group_box_layout);

  QPushButton *button_reset = new QPushButton("reset", m_central_widget);
  connect(button_reset, SIGNAL(clicked()), this, SLOT(on_reset()));

  QVBoxLayout *buttons_layout = new QVBoxLayout();
  m_score_label = new QLabel(m_central_widget);
  m_score_label->setText(QString::number(m_game.score()));
  buttons_layout->addWidget(m_score_label);
  buttons_layout->addWidget(groupBox);
  buttons_layout->addStretch();
  buttons_layout->addWidget(button_reset);  

  QHBoxLayout* main_layout = new QHBoxLayout(m_central_widget);
  main_layout->addWidget(m_paint_widget);
  main_layout->addLayout(buttons_layout);
  main_layout->setStretch(0, 1);
  setFixedSize(620, 330);

  m_paint_widget->setFocus();
  setTabOrder(m_paint_widget, m_paint_widget);
  m_timer.start();
}

MainWindow::~MainWindow() = default;

void MainWindow::on_timer()
{
  if ( m_game.done() ) {
    m_timer.stop();
    return;
  }
  if ( m_current_ai == nullptr ) { // играет пользователь    
    if ( m_actions.empty() ) {
      m_game.step(0);
    } else {
      m_game.step(m_actions.front());
      m_actions.pop();
    }
  } else { // играет ИИ
    m_current_ai->play();
  }  
  m_score_label->setText(QString::number(m_game.score()));
  repaint();
}

void MainWindow::on_up()
{
  if ( m_current_ai != nullptr ) 
    return;
  if ( !m_game.done() )
    m_actions.push(1);
}
void MainWindow::on_down()
{
  if ( m_current_ai != nullptr ) 
    return;
  if ( !m_game.done() )
    m_actions.push(2);
}

void MainWindow::on_reset()
{
  m_game.reset();
  m_timer.start();
  m_paint_widget->setFocus();
}

void MainWindow::on_state_changed(bool)
{
  QRadioButton * check_box = qobject_cast<QRadioButton*>(sender());
  assert(check_box);
  QString ai_type = check_box->text();
  if ( ai_type == QString("user") ) {
    m_current_ai.reset();
  } else {
    m_current_ai = get_ai(ai_type.toStdString());
    assert(m_current_ai != nullptr);
  } 
  m_paint_widget->setFocus();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
  QMainWindow::closeEvent(event);
}

void MainWindow::keyPressEvent(QKeyEvent *key)
{
  if ( key->key() == Qt::Key_Up ) {
    on_up();
  } else if ( key->key() == Qt::Key_Down ) {
    on_down();
  }
}

std::shared_ptr<AI> MainWindow::get_ai(std::string name)
{
  auto it = m_name2ai.find(name);
  if ( it != m_name2ai.end() ) {
    return it->second;
  } 

  std::shared_ptr<AI> ai;
  if ( name == "simple" ) {
    ai.reset(new SimpleAi(&m_game));
  } else if ( name == "reinforce" ) {
    ai.reset(new ReinforceAi(&m_game));
  } else if ( name == "network" ) {
    ai.reset(new NetworkAi(&m_game));
  }
  m_name2ai.insert(std::make_pair(name, ai));
  return ai;
}

//------------------------------------------------------------------------------
/// Отрисовать текущее состояние игры
void MyPaintWidget::paintEvent(QPaintEvent *)
{
  QPainter p(this);
  QPen pen;
  pen.setWidth(5);
  p.setPen(pen);  

  p.drawLine(game->left(),  game->top(),    game->right(), game->top());
  p.drawLine(game->right(), game->top(),    game->right(), game->bottom());
  p.drawLine(game->right(), game->bottom(), game->left(),  game->bottom());

  pen.setWidth(2);
  p.setPen(pen);
  
  QPoint ball_coords(game->ball_coords().first, game->ball_coords().second);
  p.drawEllipse(ball_coords, game->ball_radius(), game->ball_radius());
  
  p.drawLine(game->left(), game->platform_position() - game->platform_size(),
             game->left(), game->platform_position() + game->platform_size());
}