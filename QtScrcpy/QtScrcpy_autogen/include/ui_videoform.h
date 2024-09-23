/********************************************************************************
** Form generated from reading UI file 'videoform.ui'
**
** Created by: Qt User Interface Compiler version 5.15.15
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_VIDEOFORM_H
#define UI_VIDEOFORM_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <keepratiowidget.h>

QT_BEGIN_NAMESPACE

class Ui_videoForm
{
public:
    QVBoxLayout *verticalLayout;
    KeepRatioWidget *keepRatioWidget;

    void setupUi(QWidget *videoForm)
    {
        if (videoForm->objectName().isEmpty())
            videoForm->setObjectName(QString::fromUtf8("videoForm"));
        videoForm->resize(400, 800);
        videoForm->setAcceptDrops(true);
        videoForm->setStyleSheet(QString::fromUtf8("#videoForm {\n"
"	border-image: url(:/res/phone-v.png) 150px 142px 85px 142px;\n"
"	border-width: 150px 142px 85px 142px;\n"
"}"));
        verticalLayout = new QVBoxLayout(videoForm);
        verticalLayout->setSpacing(0);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        keepRatioWidget = new KeepRatioWidget(videoForm);
        keepRatioWidget->setObjectName(QString::fromUtf8("keepRatioWidget"));

        verticalLayout->addWidget(keepRatioWidget);


        retranslateUi(videoForm);

        QMetaObject::connectSlotsByName(videoForm);
    } // setupUi

    void retranslateUi(QWidget *videoForm)
    {
        videoForm->setWindowTitle(QString());
    } // retranslateUi

};

namespace Ui {
    class videoForm: public Ui_videoForm {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_VIDEOFORM_H
