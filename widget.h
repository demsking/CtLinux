/**
 * File:   Widget.cpp
 * Author: Hans Sébastien DEMANOU <demanohans@gmail.com>
 *
 * Created on 20 december 2011, 07:40
 */

#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QAction>
#include <QTextEdit>
#include <QProcess>
#include <QIcon>
#include <QTime>
#include <QTimer>
#include <QMenu>
#include <QSystemTrayIcon>
#include <QSettings>

#define FILENAME_WVDIAL_TPL      "conf/wvdial.tpl"
#define FILENAME_WVDIAL_CONF     "conf/wvdial.conf"
#define FILENAME_WVDIAL_CONF_CUR "conf/wvdial.conf.cur"

#define COMMAND_WVDIAL           "wvdial"
#define COMMAND_WVDIAL_ARG       "camtel"

namespace Ui {
    class Widget;
}

class Widget : public QWidget {
    Q_OBJECT

    public:
        explicit Widget(QWidget *parent = 0);
        ~Widget();

    private slots:
        void on_checkBox_clicked(bool checked);
        void on_nomDUtilisateurLineEdit_textChanged(const QString &);
        void on_motDePasseLineEdit_textChanged(const QString &);
        void on_numRoDeTLPhoneLineEdit_textChanged(const QString &);
        void on_nomDeLInterfaceLineEdit_textChanged(const QString &);
        void on_actionShow_activated();
        void on_actionQuit_activated();
        void processWvdial_stateChanged(QProcess::ProcessState);
        void processWvdial_readyReadStandardOutput();
        void processIfconfig_readyReadStandardOutput();
        void processPppstats_readyReadStandardOutput();
        void timerModem_update();
        void timerIP_update();
        void timerTrafic_update();
        void timerTimeConnexion_update();
        void tray_activated(QSystemTrayIcon::ActivationReason);
        void on_pushButton_save_clicked();
        void on_pushButton_connexion_clicked();
        void closeEvent(QCloseEvent *);
        void readSettings();

private:
        Ui::Widget *ui;
        QAction *actionShow;
        QAction *actionConnexion;
        QAction *actionQuit;
        QProcess *processWvdial;
        QProcess *processIfconfig;
        QProcess *processPppstats;
        QTimer *timerModem;
        QTimer *timerIP;
        QTimer *timerTrafic;
        QTimer *timerTimeConnexion;
        QIcon iconLogo;
        QIcon iconON;
        QIcon iconOFF;
        int modemConnected;
        bool connected;
        int configState;
        QTime timeConnexion;
        int o_in, o_out, p_in, p_out;
        QSystemTrayIcon *tray;
        QMenu *contextMenu;
        QSettings *settings;

        int check();
        bool end_connexion();
        void enablePushButtonConnexion();
};

#endif // WIDGET_H
