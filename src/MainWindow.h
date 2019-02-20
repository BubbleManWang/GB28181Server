#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeWidgetItem>

#include "GB28181Server/GB28181Server.h"

namespace Ui {
class MainWindow;
}

struct MessageNode
{
    QString deviceID;
    MessageType type;
    QString msgBody;
};

class MainWindow : public QMainWindow, GB28181Server
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void deviceRegisted(DeviceNode deviceNode); //�豸ע��ɹ�
    void deviceUpdate(DeviceNode deviceNode);   //�豸���£�catalog���󷵻ص��豸��Ϣ����
    void receiveMessage(const char *deviceID, MessageType type, char *msgBody);  //���յ���Ϣ

    /// �������������������߳��е��õģ���˲����ں�����ֱ�Ӳ������棬
    /// ��Ҫת�Ƶ����̺߳����в����������źŲ۵ķ�ʽ��ת�Ƶ����߳�
signals:
    void sig_deviceRegisted(DeviceNode deviceNode); //�豸ע��ɹ�
    void sig_deviceUpdate(DeviceNode deviceNode);   //�豸���£�catalog���󷵻ص��豸��Ϣ����
    void sig_receiveMessage(QString deviceID, MessageType type, QString msgBody);  //���յ���Ϣ

private slots:
    void slotDeviceRegisted(DeviceNode deviceNode); //�豸ע��ɹ�
    void slotDeviceUpdate(DeviceNode deviceNode);   //�豸���£�catalog���󷵻ص��豸��Ϣ����
    void slotReceiveMessage(QString deviceID, MessageType type, QString msgBody);  //���յ���Ϣ

private:
    Ui::MainWindow *ui;

    QMenu* mDevicePopMenu;
    QAction *mCatalogAction;

    QTreeWidgetItem *mCurrentSelectedItem; //��¼��ǰѡ�е���Ŀ

    QList<MessageNode> mMessageList;

private slots:
    void slotBtnClicked(bool isChecked);
    void slotItemClicked(QTreeWidgetItem *item, int column);
    void slotActionTriggered(bool checked);

};

#endif // MAINWINDOW_H
