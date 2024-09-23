/********************************************************************************
** Form generated from reading UI file 'toolform.ui'
**
** Created by: Qt User Interface Compiler version 5.15.15
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TOOLFORM_H
#define UI_TOOLFORM_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ToolForm
{
public:
    QVBoxLayout *verticalLayout;
    QPushButton *groupControlBtn;
    QPushButton *fullScreenBtn;
    QSpacerItem *verticalSpacer;
    QPushButton *expandNotifyBtn;
    QPushButton *touchBtn;
    QPushButton *openScreenBtn;
    QPushButton *closeScreenBtn;
    QPushButton *powerBtn;
    QPushButton *volumeUpBtn;
    QPushButton *volumeDownBtn;
    QPushButton *appSwitchBtn;
    QPushButton *menuBtn;
    QPushButton *homeBtn;
    QPushButton *returnBtn;
    QPushButton *screenShotBtn;

    void setupUi(QWidget *ToolForm)
    {
        if (ToolForm->objectName().isEmpty())
            ToolForm->setObjectName(QString::fromUtf8("ToolForm"));
        ToolForm->resize(63, 537);
        ToolForm->setStyleSheet(QString::fromUtf8(""));
        verticalLayout = new QVBoxLayout(ToolForm);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(-1, 30, -1, -1);
        groupControlBtn = new QPushButton(ToolForm);
        groupControlBtn->setObjectName(QString::fromUtf8("groupControlBtn"));

        verticalLayout->addWidget(groupControlBtn);

        fullScreenBtn = new QPushButton(ToolForm);
        fullScreenBtn->setObjectName(QString::fromUtf8("fullScreenBtn"));

        verticalLayout->addWidget(fullScreenBtn);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);

        expandNotifyBtn = new QPushButton(ToolForm);
        expandNotifyBtn->setObjectName(QString::fromUtf8("expandNotifyBtn"));

        verticalLayout->addWidget(expandNotifyBtn);

        touchBtn = new QPushButton(ToolForm);
        touchBtn->setObjectName(QString::fromUtf8("touchBtn"));

        verticalLayout->addWidget(touchBtn);

        openScreenBtn = new QPushButton(ToolForm);
        openScreenBtn->setObjectName(QString::fromUtf8("openScreenBtn"));

        verticalLayout->addWidget(openScreenBtn);

        closeScreenBtn = new QPushButton(ToolForm);
        closeScreenBtn->setObjectName(QString::fromUtf8("closeScreenBtn"));

        verticalLayout->addWidget(closeScreenBtn);

        powerBtn = new QPushButton(ToolForm);
        powerBtn->setObjectName(QString::fromUtf8("powerBtn"));

        verticalLayout->addWidget(powerBtn);

        volumeUpBtn = new QPushButton(ToolForm);
        volumeUpBtn->setObjectName(QString::fromUtf8("volumeUpBtn"));

        verticalLayout->addWidget(volumeUpBtn);

        volumeDownBtn = new QPushButton(ToolForm);
        volumeDownBtn->setObjectName(QString::fromUtf8("volumeDownBtn"));

        verticalLayout->addWidget(volumeDownBtn);

        appSwitchBtn = new QPushButton(ToolForm);
        appSwitchBtn->setObjectName(QString::fromUtf8("appSwitchBtn"));

        verticalLayout->addWidget(appSwitchBtn);

        menuBtn = new QPushButton(ToolForm);
        menuBtn->setObjectName(QString::fromUtf8("menuBtn"));

        verticalLayout->addWidget(menuBtn);

        homeBtn = new QPushButton(ToolForm);
        homeBtn->setObjectName(QString::fromUtf8("homeBtn"));

        verticalLayout->addWidget(homeBtn);

        returnBtn = new QPushButton(ToolForm);
        returnBtn->setObjectName(QString::fromUtf8("returnBtn"));

        verticalLayout->addWidget(returnBtn);

        screenShotBtn = new QPushButton(ToolForm);
        screenShotBtn->setObjectName(QString::fromUtf8("screenShotBtn"));

        verticalLayout->addWidget(screenShotBtn);


        retranslateUi(ToolForm);

        QMetaObject::connectSlotsByName(ToolForm);
    } // setupUi

    void retranslateUi(QWidget *ToolForm)
    {
        ToolForm->setWindowTitle(QCoreApplication::translate("ToolForm", "Tool", nullptr));
#if QT_CONFIG(tooltip)
        groupControlBtn->setToolTip(QCoreApplication::translate("ToolForm", "group control", nullptr));
#endif // QT_CONFIG(tooltip)
        groupControlBtn->setText(QString());
#if QT_CONFIG(tooltip)
        fullScreenBtn->setToolTip(QCoreApplication::translate("ToolForm", "full screen", nullptr));
#endif // QT_CONFIG(tooltip)
        fullScreenBtn->setText(QString());
#if QT_CONFIG(tooltip)
        expandNotifyBtn->setToolTip(QCoreApplication::translate("ToolForm", "expand notify", nullptr));
#endif // QT_CONFIG(tooltip)
        expandNotifyBtn->setText(QString());
#if QT_CONFIG(tooltip)
        touchBtn->setToolTip(QCoreApplication::translate("ToolForm", "touch switch", nullptr));
#endif // QT_CONFIG(tooltip)
        touchBtn->setText(QString());
#if QT_CONFIG(tooltip)
        openScreenBtn->setToolTip(QCoreApplication::translate("ToolForm", "open screen", nullptr));
#endif // QT_CONFIG(tooltip)
        openScreenBtn->setText(QString());
#if QT_CONFIG(tooltip)
        closeScreenBtn->setToolTip(QCoreApplication::translate("ToolForm", "close screen", nullptr));
#endif // QT_CONFIG(tooltip)
        closeScreenBtn->setText(QString());
#if QT_CONFIG(tooltip)
        powerBtn->setToolTip(QCoreApplication::translate("ToolForm", "power", nullptr));
#endif // QT_CONFIG(tooltip)
        powerBtn->setText(QString());
#if QT_CONFIG(tooltip)
        volumeUpBtn->setToolTip(QCoreApplication::translate("ToolForm", "volume up", nullptr));
#endif // QT_CONFIG(tooltip)
        volumeUpBtn->setText(QString());
#if QT_CONFIG(tooltip)
        volumeDownBtn->setToolTip(QCoreApplication::translate("ToolForm", "volume down", nullptr));
#endif // QT_CONFIG(tooltip)
        volumeDownBtn->setText(QString());
#if QT_CONFIG(tooltip)
        appSwitchBtn->setToolTip(QCoreApplication::translate("ToolForm", "app switch", nullptr));
#endif // QT_CONFIG(tooltip)
        appSwitchBtn->setText(QString());
#if QT_CONFIG(tooltip)
        menuBtn->setToolTip(QCoreApplication::translate("ToolForm", "menu", nullptr));
#endif // QT_CONFIG(tooltip)
        menuBtn->setText(QString());
#if QT_CONFIG(tooltip)
        homeBtn->setToolTip(QCoreApplication::translate("ToolForm", "home", nullptr));
#endif // QT_CONFIG(tooltip)
        homeBtn->setText(QString());
#if QT_CONFIG(tooltip)
        returnBtn->setToolTip(QCoreApplication::translate("ToolForm", "return", nullptr));
#endif // QT_CONFIG(tooltip)
        returnBtn->setText(QString());
#if QT_CONFIG(tooltip)
        screenShotBtn->setToolTip(QCoreApplication::translate("ToolForm", "screen shot", nullptr));
#endif // QT_CONFIG(tooltip)
        screenShotBtn->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class ToolForm: public Ui_ToolForm {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TOOLFORM_H
