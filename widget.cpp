/**
 * File:   Widget.cpp
 * Author: Hans Sébastien DEMANOU <demanohans@gmail.com>
 *
 * Created on 20 december 2011, 07:40
 */

#include "widget.h"
#include "ui_widget.h"
#include <QFile>
#include <QMessageBox>
#include <math.h>

Widget::Widget(QWidget *parent) : QWidget(parent), ui(new Ui::Widget) {
    ui->setupUi(this);

    modemConnected = 0;
    configState = 5;
    connected = false;

    o_in = 0;
    o_out = 0;
    p_in = 0;
    p_out = 0;

    // time and timer
    timerTimeConnexion = new QTimer(this);
    timerModem = new QTimer(this);
    timerIP = new QTimer(this);
    timerTrafic = new QTimer(this);

    timerTimeConnexion->setInterval(1000);
    timerModem->setInterval(1000);
    timerIP->setInterval(1000);
    timerTrafic->setInterval(1000);

    // icon
    iconLogo.addFile(QString::fromUtf8(":/png/resource/icon.png"), QSize(), QIcon::Normal, QIcon::On);
    iconOFF.addFile(QString::fromUtf8(":/png/resource/1315493472_system-red.png"), QSize(), QIcon::Normal, QIcon::Off);
    iconON.addFile(QString::fromUtf8(":/png/resource/1315493431_system-green.png"), QSize(), QIcon::Normal, QIcon::On);

    // tray
    tray = new QSystemTrayIcon(iconLogo, this);
    tray->setToolTip(this->windowTitle());
    tray->show();

    contextMenu = new QMenu(this);
    actionShow = contextMenu->addAction(QApplication::translate("Widget", "Masquer", 0, QApplication::UnicodeUTF8));
    actionConnexion = contextMenu->addAction(
                !connected ?
                    QApplication::translate("Widget", "Se connecter", 0, QApplication::UnicodeUTF8) :
                    QApplication::translate("Widget", "Se déconnecter", 0, QApplication::UnicodeUTF8));
    actionQuit = contextMenu->addAction(QApplication::translate("Widget", "Quitter", 0, QApplication::UnicodeUTF8));
    tray->setContextMenu(contextMenu);

    timerModem->start();

    // process
    processWvdial = new QProcess(this);
    processIfconfig = new QProcess(this);
    processPppstats = new QProcess(this);

    // connect
    connect(actionShow, SIGNAL(activated()),
            this, SLOT(on_actionShow_activated()));
    connect(actionConnexion, SIGNAL(activated()),
            this, SLOT(on_pushButton_connexion_clicked()));
    connect(actionQuit, SIGNAL(activated()),
            this, SLOT(on_actionQuit_activated()));
    connect(ui->pushButton_save, SIGNAL(clicked()),
            this, SLOT(on_actionSaveSetting_activated()));
    connect(ui->nomDUtilisateurLineEdit, SIGNAL(textChanged(const QString &)),
            this, SLOT(on_nomDUtilisateurLineEdit_textChanged(const QString &)));
    connect(ui->motDePasseLineEdit, SIGNAL(textChanged(const QString &)),
            this, SLOT(on_motDePasseLineEdit_textChanged(const QString &)));
    connect(ui->numRoDeTLPhoneLineEdit, SIGNAL(textChanged(const QString &)),
            this, SLOT(on_numRoDeTLPhoneLineEdit_textChanged(const QString &)));
    connect(ui->nomDeLInterfaceLineEdit, SIGNAL(textChanged(const QString &)),
            this, SLOT(on_nomDeLInterfaceLineEdit_textChanged(const QString &)));
    connect(processWvdial, SIGNAL(readyReadStandardOutput()),
            this, SLOT(processWvdial_readyReadStandardOutput()));
    connect(processWvdial, SIGNAL(stateChanged(QProcess::ProcessState)),
            this, SLOT(processWvdial_stateChanged(QProcess::ProcessState)));
    connect(processIfconfig, SIGNAL(readyReadStandardOutput()),
            this, SLOT(processIfconfig_readyReadStandardOutput()));
    connect(processPppstats, SIGNAL(readyReadStandardOutput()),
            this, SLOT(processPppstats_readyReadStandardOutput()));
    connect(timerModem, SIGNAL(timeout()), this, SLOT(timerModem_update()));
    connect(timerIP, SIGNAL(timeout()), this, SLOT(timerIP_update()));
    connect(timerTrafic, SIGNAL(timeout()), this, SLOT(timerTrafic_update()));
    connect(timerTimeConnexion, SIGNAL(timeout()),
            this, SLOT(timerTimeConnexion_update()));
    connect(tray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(tray_activated(QSystemTrayIcon::ActivationReason)));

    // loading
    settings = new QSettings("conf/settings.ini", QSettings::IniFormat, this);
    readSettings();
    if (check() != 0) {
        ui->tabWidget->setCurrentIndex(1);
    }
}

Widget::~Widget() {
    delete ui;
}

void Widget::closeEvent(QCloseEvent *event) {
    settings->setValue("Widget/geometry", saveGeometry());
    if (processWvdial->state() == QProcess::Running) {
        if (end_connexion()) {
            QWidget::closeEvent(event);
        }
    } else {
        QWidget::closeEvent(event);
    }
}

void Widget::readSettings() {
    restoreGeometry(settings->value("Widget/geometry").toByteArray());
    ui->nomDUtilisateurLineEdit->setText(settings->value("config/username").toString());
    ui->motDePasseLineEdit->setText(settings->value("config/password").toString());
    ui->numRoDeTLPhoneLineEdit->setText(settings->value("config/phone").toString());
    ui->nomDeLInterfaceLineEdit->setText(settings->value("config/interface").toString());
    ui->checkBox_autoconnect->setChecked(settings->value("config/autoconnect").toBool());
    ui->checkBox_reconnect->setChecked(settings->value("config/reconnect").toBool());
}

void Widget::on_actionShow_activated() {
    this->setVisible(!this->isVisible());
    actionShow->setText(this->isVisible() ? QApplication::translate("Widget", "Masquer", 0, QApplication::UnicodeUTF8) : QApplication::translate("Widget", "Afficher", 0, QApplication::UnicodeUTF8));
}

void Widget::on_actionQuit_activated() {
    this->close();
}

void Widget::tray_activated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger) {
        on_actionShow_activated();
    }
}

void Widget::timerModem_update() {
    static QString dev = "/dev/";
    static int i, length = ui->comboBoxModem->count() - 1;
    static int j = 0, k;
    for (j = 0, i = length, k = 0; i > 0; i--) {
        if (QFile::exists(dev + ui->comboBoxModem->itemText(i))) {
            ui->comboBoxModem->setItemIcon(i, iconON);
            k++;
            j = i;
        } else {
            ui->comboBoxModem->setItemIcon(i, iconOFF);
        }
    }
    if (!connected && ui->comboBoxModem->currentIndex() != j) {
        ui->comboBoxModem->setCurrentIndex(j);
        modemConnected = j;
        enablePushButtonConnexion();
    }
}

void Widget::timerIP_update() {
    processIfconfig->start("ifconfig", QStringList() << ui->nomDeLInterfaceLineEdit->text());
}

void Widget::timerTrafic_update() {
    processPppstats->start("pppstats", QStringList() << ui->nomDeLInterfaceLineEdit->text());
}

void Widget::timerTimeConnexion_update() {
    timeConnexion = timeConnexion.addSecs(1);
    ui->labelTimeConnexionText->setText(timeConnexion.toString("hh:mm:ss"));
}

QString convert(double x) {
    static double powv, powvl;
    static QString val;

    if ((x / (powv = pow(2, 10))) < 1) {
        return val.sprintf("%.0f o", x);
    } else {
        powvl = x / powv;
        if (x >= (powv = pow(2, 20))) {
            powvl = x / powv;
            if (x >= (powv = pow(2, 30))) {
                powvl = x / powv;
                if (x >= (powv = pow(2, 40))) {
                    powvl = x / powv;
                    if (x >= (powv = pow(2, 50))) {
                        powvl = x / powv;
                        if (x >= (powv = pow(2, 60))) {
                            powvl = x / powv;
                            if (x >= (powv = pow(2, 70))) {
                                powvl = x / powv;
                                if (x >= (powv = pow(2, 80))) {
                                    powvl = x / powv;
                                    return val.sprintf("%.1f Yi", powvl);
                                } else {
                                    return val.sprintf("%.1f Zi", powvl);
                                }
                            } else {
                                return val.sprintf("%.1f Ei", powvl);
                            }
                        } else {
                            return val.sprintf("%.1f Pi", powvl);
                        }
                    } else {
                        return val.sprintf("%.1f Ti", powvl);
                    }
                } else {
                    return val.sprintf("%.1f Gi", powvl);
                }
            } else {
                return val.sprintf("%.1f Mi", powvl);
            }
        } else {
            return val.sprintf("%.1f Ki", powvl);
        }
    }
}

inline bool Widget::end_connexion() {
    if (QMessageBox::question(this, QApplication::translate("Widget", "Fermeture de la connexion", 0, QApplication::UnicodeUTF8),
                          QApplication::translate("Widget", "Êtes-vous sûr de vouloir vous déconnecter ?", 0, QApplication::UnicodeUTF8), QMessageBox::Ok, QMessageBox::Cancel) == QMessageBox::Ok) {
        static QString command;
        command.sprintf("pkill -15 -P %d", processWvdial->pid());
        if (QProcess::execute(command) >= 0) {
            processWvdial->close();
            return true;
        } else {
            QMessageBox::information(this, "Erreur", "Une erreur est survenue lors de la déconnexion !");
            return false;
        }
    }
}

void Widget::on_pushButton_connexion_clicked() {
    if (processWvdial->state() == QProcess::NotRunning) {
        QFile::remove(FILENAME_WVDIAL_CONF_CUR);
        if (!QFile::copy(FILENAME_WVDIAL_CONF, FILENAME_WVDIAL_CONF_CUR)) {
            QMessageBox::critical(this, QApplication::translate("Widget", "Erreur de connexion", 0, QApplication::UnicodeUTF8), QApplication::translate("Widget", "Une erreur inattendue est survenue !", 0, QApplication::UnicodeUTF8));
            return;
        }
        static QFile *wvconf = new QFile(FILENAME_WVDIAL_CONF_CUR);
        if (wvconf->open(QIODevice::Append)) {
            static char *line = new char[255];
            sprintf(line, "\nModem = /dev/%s", ui->comboBoxModem->currentText().toStdString().c_str());
            wvconf->write(line);
            wvconf->close();
        }
        processWvdial->start(COMMAND_WVDIAL + QString(" -C ") + FILENAME_WVDIAL_CONF_CUR + QString(" ") + COMMAND_WVDIAL_ARG);
    } else {
        end_connexion();
    }
}

void Widget::processWvdial_stateChanged(QProcess::ProcessState state) {
    if (state == QProcess::Running) {
        connected = true;
        ui->comboBoxModem->setEnabled(false);
        timerModem->stop();
        ui->nomDeLInterfaceLineEdit->setEnabled(false);
        ui->pushButton_connexion->setText(QApplication::translate("Widget", "Connexion en cours", 0, QApplication::UnicodeUTF8));
        actionConnexion->setText(ui->pushButton_connexion->text());
        tray->showMessage("CtLinux", ui->pushButton_connexion->text());
        ui->pushButton_connexion->setEnabled(false);
        actionConnexion->setText(ui->pushButton_connexion->text());
        actionConnexion->setEnabled(false);
        ui->labelOctetReceive->setText("0");
        ui->labelPackageReceive->setText("0");
        ui->labelOctetSend->setText("0");
        ui->labelPackageSend->setText("0");
        timerIP->start();
    } else if (state == QProcess::NotRunning) {
        connected = false;
        ui->comboBoxModem->setEnabled(true);
        ui->nomDeLInterfaceLineEdit->setEnabled(true);
        ui->pushButton_connexion->setText(QApplication::translate("Widget", "Se co&nnecter", 0, QApplication::UnicodeUTF8));
        actionConnexion->setText(ui->pushButton_connexion->text());
        ui->labelStateText->setText(QApplication::translate("Widget", "Indisponible", 0, QApplication::UnicodeUTF8));
        tray->showMessage(QApplication::translate("Widget", "Déconnecté", 0, QApplication::UnicodeUTF8), QApplication::translate("Widget", "Non connecté", 0, QApplication::UnicodeUTF8));
        timerModem->start();
        timerIP->stop();
        timerTimeConnexion->stop();
        timerTrafic->stop();
        ui->labelOctetReceive->clear();
        ui->labelOctetSend->clear();
        ui->labelPackageReceive->clear();
        ui->labelPackageSend->clear();
        ui->labelIPText->clear();
        ui->labelPAPText->clear();
        ui->labelMaskText->clear();
        timeConnexion.setHMS(0, 0, 0, 0);
        ui->labelTimeConnexionText->setText("00:00:00");
        ui->groupBoxIP->setEnabled(false);
        ui->groupBoxTrafic->setEnabled(false);
        processPppstats->close();
        o_in = 0;
        o_out = 0;
        p_in = 0;
        p_out = 0;
        ui->labelOctetReceive->setText("0");
        ui->labelPackageReceive->setText("0");
        ui->labelOctetSend->setText("0");
        ui->labelPackageSend->setText("0");
        if (!connected) {
            ui->labelInterfaceText->setText(ui->nomDeLInterfaceLineEdit->text());
        }
        enablePushButtonConnexion();
    }
}

void Widget::processWvdial_readyReadStandardOutput() {
    static QRegExp pattern;
    static QString out;

    out = processWvdial->readAll().data();

    pattern.setPattern("Cannot open /dev/ttyUSB");
    pattern.indexIn(out);

    if (pattern.capturedTexts()[0].length() != 0) {
        QMessageBox::critical(this, QApplication::translate("Widget", "Modem indisponible", 0, QApplication::UnicodeUTF8), QApplication::translate("Widget", "Le modem est indisponible.\nRaisons: \n- Le modem n'est pas connecté à l'ordinateur\n- Le modem connecté est sous tension\n- Le modem est utilisé par un autre programme", 0, QApplication::UnicodeUTF8));
    } else {
        pattern.setPattern("Authentication error");
        pattern.indexIn(out);

        if (pattern.capturedTexts()[0].length() != 0) {
            QMessageBox::critical(this, QApplication::translate("Widget", "Echec d'authentification", 0, QApplication::UnicodeUTF8), QApplication::translate("Widget", "Le nom de l'utilisateur ou le mot de passe est incorrect", 0, QApplication::UnicodeUTF8));
        } else {
            pattern.setPattern("NO CARRIER");
            pattern.indexIn(out);

            if (pattern.capturedTexts()[0].length() != 0) {
                QMessageBox::critical(this, QApplication::translate("Widget", "Numéro de téléphone éroné", 0, QApplication::UnicodeUTF8), QApplication::translate("Widget", "Impossible de contacter l'opérateur à partir du numéro de téléphone de la configuration", 0, QApplication::UnicodeUTF8));
            }
        }
    }
}

void Widget::processIfconfig_readyReadStandardOutput() {
    if (ui->labelIPText->text().isEmpty()) {
        QString output = processIfconfig->readAll().data();

        if (!output.isEmpty()) {
            timeConnexion.setHMS(0, 0, 0, 0);
            timerTimeConnexion->start();
            processIfconfig->close();

            static QRegExp pattern("(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})[- a-zA-Z:]+(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})[- a-zA-Z:]+(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})");
            static QStringList line;
            pattern.indexIn(output);
            line = pattern.capturedTexts();

            ui->labelIPText->setText(line.at(1));
            ui->labelPAPText->setText(line.at(2));
            ui->labelMaskText->setText(line.at(3));

            if (!ui->labelIPText->text().isEmpty()) {
                timerIP->stop();
                ui->groupBoxIP->setEnabled(true);
                ui->groupBoxTrafic->setEnabled(true);
                ui->pushButton_connexion->setText(QApplication::translate("Widget", "Se &déconnecter", 0, QApplication::UnicodeUTF8));
                actionConnexion->setText(QApplication::translate("Widget", "Se déconnecter", 0, QApplication::UnicodeUTF8));
                ui->labelStateText->setText(QApplication::translate("Widget", "Connecté", 0, QApplication::UnicodeUTF8));
                ui->pushButton_connexion->setEnabled(true);
                actionConnexion->setEnabled(true);
                tray->showMessage("CtLinux", ui->labelStateText->text() + " en tant que " + ui->nomDUtilisateurLineEdit->text());
            }

            timerTrafic->start();
        }
    }
}

void Widget::processPppstats_readyReadStandardOutput() {
    static QStringList line;
    static QRegExp pattern("(\\d+) *(\\d+)( *\\d+)+[ |]+(\\d+) *(\\d+)");

    pattern.indexIn(processPppstats->readAllStandardOutput().data());

    if (pattern.captureCount() == 5) {
        line = pattern.capturedTexts();

        ui->labelOctetReceive->setText(convert(line.at(1).toDouble()));
        ui->labelPackageReceive->setText(line.at(2));
        ui->labelOctetSend->setText(convert(line.at(4).toDouble()));
        ui->labelPackageSend->setText(line.at(5));
    }
}

inline void Widget::enablePushButtonConnexion() {
    if (modemConnected) {
        if (!connected) {
            ui->pushButton_connexion->setEnabled(configState == 0);
            actionConnexion->setEnabled(ui->pushButton_connexion->isEnabled());
        }
    } else {
        ui->pushButton_connexion->setEnabled(false);
        actionConnexion->setEnabled(false);
    }
}

inline int Widget::check() {
    if (ui->nomDUtilisateurLineEdit->text().isEmpty()
            || ui->motDePasseLineEdit->text().isEmpty()
            || ui->numRoDeTLPhoneLineEdit->text().isEmpty()
            || ui->nomDeLInterfaceLineEdit->text().isEmpty()) {
        configState = 1;
        ui->labelInfo->setVisible(true);
    } else {
        configState = 0;
        ui->labelInfo->setVisible(false);
    }
    enablePushButtonConnexion();

    return configState;
}

void Widget::on_checkBox_clicked(bool checked) {
    this->ui->motDePasseLineEdit->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
}

void Widget::on_nomDUtilisateurLineEdit_textChanged(const QString &value) {
    ui->nomDUtilisateurLineEdit->setText(value.trimmed());
}

void Widget::on_motDePasseLineEdit_textChanged(const QString &value) {
    ui->motDePasseLineEdit->setText(value.trimmed());
}

void Widget::on_numRoDeTLPhoneLineEdit_textChanged(const QString &value) {
    ui->numRoDeTLPhoneLineEdit->setText(value.trimmed());
}

void Widget::on_nomDeLInterfaceLineEdit_textChanged(const QString &value) {
    ui->nomDeLInterfaceLineEdit->setText(value.trimmed());
    if (!connected) {
        ui->labelInterfaceText->setText(ui->nomDeLInterfaceLineEdit->text());
    }
}

void Widget::on_pushButton_save_clicked() {
    settings->setValue("config/username", ui->nomDUtilisateurLineEdit->text());
    settings->setValue("config/password", ui->motDePasseLineEdit->text());
    settings->setValue("config/phone", ui->numRoDeTLPhoneLineEdit->text());
    settings->setValue("config/interface", ui->nomDeLInterfaceLineEdit->text());
    settings->setValue("config/autoconnect", ui->checkBox_autoconnect->isChecked());
    settings->setValue("config/reconnect", ui->checkBox_reconnect->isChecked());

    check();

    // Enregistrement du fichier de configuration wvdial.conf
    QFile::remove(FILENAME_WVDIAL_CONF);
    QFile::copy(FILENAME_WVDIAL_TPL, FILENAME_WVDIAL_CONF);

    static QFile *wvconf = new QFile(FILENAME_WVDIAL_CONF);
    if (wvconf->open(QIODevice::Append)) {
        static QString data;
        data.sprintf("Username = \"%s\"\nPassword = \"%s\"\nPhone = \"%s\"\n",
            ui->nomDUtilisateurLineEdit->text().toStdString().c_str(),
            ui->motDePasseLineEdit->text().toStdString().c_str(),
            ui->numRoDeTLPhoneLineEdit->text().toStdString().c_str()
        );
        wvconf->write(data.toStdString().c_str());
        wvconf->close();
    }
}
