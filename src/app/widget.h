#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QSettings>
#include <QCloseEvent>
#include <QString>

#include "global.h"

namespace Ui {
    class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget* parent = nullptr);
    ~Widget();

private:
    Ui::Widget* ui;
    QSettings* settings;

    void closeEvent(QCloseEvent* event);
    void loadSettings();

    QString findInstallDirectory() const;
    QString getInstallDirectory(bool warning = true);
    void populateComboboxWithInstallDirectories() const;
    void populateComboboxWithNetworkInterfaces() const;
    void updatePatchStatus(bool patched) const;

    void log(const QString &message);

private slots:
    void saveSettings() const;

    void pushButton_install_directory_clicked();
    void comboBox_network_interface_currentIndexChanged(int index);
    void pushButton_patch_clicked();
    void on_pushButton_clicked();
};

#endif // WIDGET_H
