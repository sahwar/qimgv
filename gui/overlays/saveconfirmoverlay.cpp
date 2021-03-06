#include "saveconfirmoverlay.h"
#include "ui_saveconfirmoverlay.h"

SaveConfirmOverlay::SaveConfirmOverlay(OverlayContainerWidget *parent) :
    FloatingWidget(parent),
    ui(new Ui::SaveConfirmOverlay)
{
    this->position = FloatingWidgetPosition::BOTTOMRIGHT;
    ui->setupUi(this);
    connect(ui->saveButton, SIGNAL(clicked()), this, SIGNAL(saveClicked()));
    connect(ui->saveAsButton, SIGNAL(clicked()), this, SIGNAL(saveAsClicked()));
    connect(ui->discardButton, SIGNAL(clicked()), this, SIGNAL(discardClicked()));
    this->setFocusPolicy(Qt::NoFocus);
    this->hide();
}

SaveConfirmOverlay::~SaveConfirmOverlay()
{
    delete ui;
}
