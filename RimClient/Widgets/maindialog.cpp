﻿#include "maindialog.h"

#include <QDesktopWidget>
#include <QMouseEvent>
#include <QTimer>
#include <QHBoxLayout>
#include <QApplication>
#include <QToolButton>

#include "systemtrayicon.h"
#include "Util/rutil.h"
#include "constants.h"
#include "head.h"
#include "toolbar.h"
#include "panelbottomtoolbar.h"
#include "panelcontentarea.h"
#include "paneltoparea.h"
#include "actionmanager/actionmanager.h"
#include "Util/imagemanager.h"
#include "toolbox/toolitem.h"
#include "Util/rsingleton.h"
#include "actionmanager/shortcutsettings.h"
#include "editpersoninfowindow.h"

#include "abstractchatwidget.h"
#include "itemhoverinfo.h"

#define PANEL_MARGIN 20

class MainDialogPrivate : public GlobalData<MainDialog>
{
    Q_DECLARE_PUBLIC(MainDialog)

    MainDialogPrivate(MainDialog *q):q_ptr(q)
    {
        editWindow = NULL;
    }

private:
    QWidget * MainPanel;
    QWidget * TopBar;
    QWidget * Conent;
    QWidget * ToolBarWidget;

    ToolBar * toolBar;
    PanelBottomToolBar * bottomToolBar;
    PanelContentArea * panelContentArea;
    PanelTopArea * panelTopArea;

    QMap<ToolItem * ,AbstractChatWidget*> chatWidgets;
    QMap<ToolItem * ,ItemHoverInfo *> hoverInfos;
    EditPersonInfoWindow * editWindow;

    MainDialog * q_ptr;
};

MainDialog * MainDialog::dialog = NULL;

MainDialog::MainDialog(QWidget *parent) :
    d_ptr(new MainDialogPrivate(this)),
    Widget(parent)
{
    setMinimumSize(Constant::MAIN_PANEL_MIN_WIDTH,Constant::MAIN_PANEL_MIN_HEIGHT);
    setMaximumWidth(Constant::MAIN_PANEL_MAX_WIDTH);
    setMaximumHeight(qApp->desktop()->screen()->height());

    dialog = this;
    RSingleton<Subject>::instance()->attach(this);

    initWidget();
}

MainDialog::~MainDialog()
{
    MQ_D(MainDialog);
    if(d->editWindow)
    {
        delete d->editWindow;
    }

    if(d->chatWidgets.size() > 0)
    {
       QMap<ToolItem *,AbstractChatWidget*>::const_iterator iter =  d->chatWidgets.begin();
       while(iter != d->chatWidgets.end())
       {
           delete iter.value();
           iter++;
       }
    }

    RSingleton<ShortcutSettings>::instance()->save();
}

MainDialog *MainDialog::instance()
{
    return dialog;
}

void MainDialog::onMessage(MessageType)
{

}

void MainDialog::resizeEvent(QResizeEvent *)
{
    updateWidgetGeometry();
}

void MainDialog::closeEvent(QCloseEvent *)
{
    writeSettings();
    //Yang 延时退出，否则ini文件无法被更新至本地
    QTimer::singleShot(50,qApp,SLOT(quit()));
}

void MainDialog::updateWidgetGeometry()
{
    MQ_D(MainDialog);
    QLayout * lay = layout();
    int right = 0;
    int left  = 0;
    if(lay)
    {
        QMargins margins = lay->contentsMargins();
        right = margins.right();
        left = margins.left();
    }
    d->toolBar->setGeometry(left,0,this->width() - right * 3,Constant::TOOL_HEIGHT);
}

void MainDialog::closeWindow()
{
    this->close();
}

void MainDialog::makeWindowFront(bool flag)
{
    Qt::WindowFlags flags = windowFlags();

    if(flag)
    {
        setWindowFlags(flags | Qt::WindowStaysOnTopHint);
        ActionManager::instance()->toolButton(Id(Constant::TOOL_PANEL_FRONT))->setToolTip(tr("Unstick"));
    }
    else
    {
        setWindowFlags(flags & ~Qt::WindowStaysOnTopHint);
        ActionManager::instance()->toolButton(Id(Constant::TOOL_PANEL_FRONT))->setToolTip(tr("Stick"));
    }

    RUtil::globalSettings()->setValue("Main/topHint",flag);

    show();
}

void MainDialog::showChatWindow(ToolItem * item)
{
    MQ_D(MainDialog);
    AbstractChatWidget * widget = NULL;
    if(d->chatWidgets.contains(item))
    {
        widget = d->chatWidgets.value(item);
    }
    else
    {
       widget = new AbstractChatWidget();

       d->chatWidgets.insert(item,widget);
    }

    widget->show();
}

void MainDialog::showHoverItem(bool flag, ToolItem * item)
{
    MQ_D(MainDialog);
    if(flag)
    {
        ItemHoverInfo * info = new ItemHoverInfo;
        info->fadein(item->mapToGlobal(QPoint(0,0)));
        d->hoverInfos.insert(item,info);
    }
    else
    {
        ItemHoverInfo  *info = d->hoverInfos.value(item);
        if(info)
        {
            d->hoverInfos.remove(item);
            info->fadeout();
        }
    }
}

void MainDialog::showPersonalEditWindow()
{
    MQ_D(MainDialog);
    if(!d->editWindow)
    {
        d->editWindow = new EditPersonInfoWindow();
        connect(d->editWindow,SIGNAL(destroyed(QObject*)),this,SLOT(updateEditInstance()));
    }
    if(d->editWindow->isMinimized())
    {
        d->editWindow->showNormal();
    }
    else
    {
        d->editWindow->show();
    }
}

void MainDialog::updateEditInstance()
{
   MQ_D(MainDialog);
   d->editWindow = NULL;
}

#include <QToolButton>

void MainDialog::initWidget()
{
    MQ_D(MainDialog);
    d->MainPanel = new QWidget(this);
    d->MainPanel->setObjectName("MainPanel");

    setContentWidget(d->MainPanel);

    d->TopBar = new QWidget(this);
    d->TopBar->setFixedHeight(150);
    d->TopBar->setObjectName("TopBar");

    d->Conent = new QWidget(this);
    d->Conent->setObjectName("Conent");

    d->ToolBarWidget = new QWidget(this);
    d->ToolBarWidget->setObjectName("ToolBarWidget");

    QVBoxLayout * mainLayout = new QVBoxLayout();
    mainLayout->setContentsMargins(0,0,0,0);
    mainLayout->setSpacing(0);

    mainLayout->addWidget(d->TopBar);
    mainLayout->addWidget(d->Conent);
    mainLayout->addWidget(d->ToolBarWidget);

    d->MainPanel->setLayout(mainLayout);

    connect(SystemTrayIcon::instance(),SIGNAL(quitApp()),this,SLOT(closeWindow()));
    connect(SystemTrayIcon::instance(),SIGNAL(showMainPanel()),this,SLOT(showNormal()));

    readSettings();

    d->toolBar = new ToolBar(d->MainPanel);
    d->toolBar->setToolFlags(ToolBar::TOOL_ICON|ToolBar::TOOL_MIN|ToolBar::TOOL_CLOSE|ToolBar::TOOL_SPACER);
    connect(d->toolBar,SIGNAL(minimumWindow()),this,SLOT(showMinimized()));
    connect(d->toolBar,SIGNAL(closeWindow()),this,SLOT(closeWindow()));

    d->toolBar->setWindowIcon(RSingleton<ImageManager>::instance()->getWindowIcon(ImageManager::WHITE,ImageManager::ICON_SYSTEM_16));

    RToolButton * frontButton = ActionManager::instance()->createToolButton(Constant::TOOL_PANEL_FRONT,this,SLOT(makeWindowFront(bool)),true);

    d->toolBar->insertToolButton(frontButton,Constant::TOOL_MIN);

    makeWindowFront(RUtil::globalSettings()->value("Main/topHint").toBool());

    d->panelTopArea = new PanelTopArea(d->TopBar);
    QHBoxLayout * topAreaLayout = new QHBoxLayout();
    topAreaLayout->setContentsMargins(0,0,0,0);
    topAreaLayout->setSpacing(0);
    topAreaLayout->addWidget(d->panelTopArea);
    d->TopBar->setLayout(topAreaLayout);

    //中部内容区
    d->panelContentArea = new PanelContentArea(d->Conent);
    QHBoxLayout * contentLayout = new QHBoxLayout();
    contentLayout->setContentsMargins(0,0,0,0);
    contentLayout->setSpacing(0);
    contentLayout->addWidget(d->panelContentArea);
    d->Conent->setLayout(contentLayout);

    //底部工具栏
    d->bottomToolBar = new PanelBottomToolBar(d->ToolBarWidget);
    QHBoxLayout * bottomToolLayout = new QHBoxLayout();
    bottomToolLayout->setContentsMargins(0,0,0,0);
    bottomToolLayout->setSpacing(0);
    bottomToolLayout->addWidget(d->bottomToolBar);
    d->ToolBarWidget->setLayout(bottomToolLayout);

    QTimer::singleShot(0,this,SLOT(updateWidgetGeometry()));
}

#define SCALE_ZOOMIN_FACTOR 1.2
#define SCALE_ZOOMOUT_FACTOR 0.75

void MainDialog::readSettings()
{
    QSettings * settings = RUtil::globalSettings();

    if(!settings->value("Main/x").isValid() || !settings->value("Main/y").isValid()
            ||!settings->value("Main/width").isValid() ||!settings->value("Main/height").isValid())
    {
        QRect rect = qApp->desktop()->screen()->geometry();

        int tmpWidth = Constant::MAIN_PANEL_MIN_WIDTH * SCALE_ZOOMIN_FACTOR;
        int tmpHeight = rect.height() * SCALE_ZOOMOUT_FACTOR;


        RUtil::globalSettings()->setValue("Main/x",rect.width() - tmpWidth - PANEL_MARGIN);
        RUtil::globalSettings()->setValue("Main/y",PANEL_MARGIN);
        RUtil::globalSettings()->setValue("Main/width",tmpWidth);
        RUtil::globalSettings()->setValue("Main/height",tmpHeight);
    }

    int x = RUtil::globalSettings()->value("Main/x").toInt();
    int y = RUtil::globalSettings()->value("Main/y").toInt();
    int w = RUtil::globalSettings()->value("Main/width").toInt();
    int h = RUtil::globalSettings()->value("Main/height").toInt();

    this->setGeometry(x,y,w,h);
}

void MainDialog::writeSettings()
{
    QRect rect = this->geometry();
    RUtil::globalSettings()->setValue("Main/x",rect.x());
    RUtil::globalSettings()->setValue("Main/y",rect.y());
    RUtil::globalSettings()->setValue("Main/width",rect.width());
    RUtil::globalSettings()->setValue("Main/height",rect.height());
}
