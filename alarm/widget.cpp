#include "widget.h"
#include "ui_widget.h"

#include <QDateTime>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    initLayout();
    initTableWidget();
    initSignalImitation();
}

Widget::~Widget()
{
    delete ui;
}


// 初始化布局
void Widget::initLayout()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(ui->rb_box);
    layout->addWidget(ui->tw);
    setLayout(layout);

    setWindowTitle("Alarm");
    resize(1600, 1000);
}


// 初始化表格
void Widget::initTableWidget()
{
    QStringList headers{"Time", "Location", "Type", "Channel"};
    ui->tw->setColumnCount(headers.size());
    ui->tw->setHorizontalHeaderLabels(headers);

    QHeaderView *headerView = ui->tw->horizontalHeader();
    headerView->setSectionResizeMode(QHeaderView::Stretch);
}


// 初始化信号模拟
void Widget::initSignalImitation()
{
    QTimer *timer = new QTimer(this);
    timer->setInterval(5000);
    timer->start();

    connect(timer, &QTimer::timeout, this, [=]()
    {
        // 接收数据
        QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        QString location = QString::number(qrand()% 200);
        QString type = QString::number(qrand()% 5);
        QString channel = QString::number(qrand()% 3);
        QString cnt = "1";

        // 插入表格
        int row = ui->tw->rowCount();
        ui->tw->insertRow(row);
        ui->tw->setItem(row, 0, new QTableWidgetItem(time));
        ui->tw->setItem(row, 1, new QTableWidgetItem(location));
        ui->tw->setItem(row, 2, new QTableWidgetItem(type));
        ui->tw->setItem(row, 3, new QTableWidgetItem(channel));

        // 创建警报窗口
        if(ui->rb_box->isChecked())
        {
            AlarmWidget * aw = new AlarmWidget(qrand() % 800, qrand() % 800, {"Warn(local)", "Location(m)", "Type", "Channel", "Time"}, "alarm.wav");
            aw->setData({time, location, type, channel, cnt});
            aw->show();
        }
    });
}

