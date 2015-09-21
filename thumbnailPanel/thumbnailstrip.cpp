#include "thumbnailstrip.h"

ThumbnailStrip::ThumbnailStrip(QWidget *parent)
    : QGraphicsView(parent)
{
    panelHeight = globalSettings->thumbnailSize() + 22;
    current=-1;
    widget = new QGraphicsWidget();
    scene = new CustomScene;
    layout = new QGraphicsLinearLayout(Qt::Horizontal);
    timeLine = new QTimeLine(50, this);
    timeLine->setCurveShape(QTimeLine::EaseInCurve);
    widget->setLayout(layout);
    layout->setSpacing(0);
    layout->setContentsMargins(2,0,2,0);
    scene->addItem(widget);

    this->setScene(scene);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setFrameShape(QFrame::NoFrame);
    this->setGeometry(0,0,300,panelHeight);
    this->horizontalScrollBar()->setAttribute(Qt::WA_NoMousePropagation, true);
    this->hide();

    loadTimer.setSingleShot(true);

    connect(scene, SIGNAL(sceneClick(QPointF)),
            this, SLOT(sceneClicked(QPointF)));

    connect(this->horizontalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(loadVisibleThumbnailsDelayed()));
    connect(this->horizontalScrollBar(), SIGNAL(sliderMoved(int)),
            this, SLOT(loadVisibleThumbnailsDelayed()));
    connect(&loadTimer, SIGNAL(timeout()),
            this, SLOT(loadVisibleThumbnails()));
    connect(timeLine, SIGNAL(frameChanged(int)), horizontalScrollBar(), SLOT(setValue(int)));
}

void ThumbnailStrip::fillPanel(int count) {
    current = -1;
    loadTimer.stop();
    populate(count);
    loadVisibleThumbnailsDelayed();
    //this->horizontalScrollBar()->setValue(0);
}

void ThumbnailStrip::selectThumbnail(int pos) {
    if(current != -1) {
        thumbnailLabels.at(current)->setHighlighted(false);
        thumbnailLabels.at(current)->setOpacityAnimated(OPACITY_INACTIVE, ANIMATION_SPEED_INSTANT);
    }
    thumbnailLabels.at(pos)->setHighlighted(true);
    thumbnailLabels.at(pos)->setOpacityAnimated(OPACITY_SELECTED, ANIMATION_SPEED_FAST);
    current = pos;
    if(!childVisibleEntirely(pos)) {
        centerOn(thumbnailLabels.at(pos)->scenePos());
    }
    loadVisibleThumbnails();
}

void ThumbnailStrip::centerOnSmooth(const QPointF &pos) {
    Q_UNUSED(pos)

    timeLine->setFrameRange(0, -1000);
    timeLine->setUpdateInterval(16);
    timeLine->start();
    //centerOn(pos);
}

void ThumbnailStrip::translateX(int dx) {
    translate((qreal)dx/10, 0);
}

void ThumbnailStrip::populate(int count) {
    // this will fail if list items != layout items
    // shouldnt happen though
    for(int i = layout->count()-1; i >= 0; --i) {
        layout->removeAt(0);
        delete thumbnailLabels.takeAt(0);
    }
    thumbnailLabels.clear();

    for(int i=0; i<count; i++) {
        addItem();
    }

    layout->invalidate();
    layout->activate();
    scene->setSceneRect(layout->geometry());
}

// in theory faster than scene's version
QRectF ThumbnailStrip::itemsBoundingRect() {
    QRectF boundingRect(0,0,0,0);
    if(!thumbnailLabels.isEmpty()) {
        boundingRect.setTopLeft(
                    thumbnailLabels.at(0)->sceneBoundingRect().topLeft());
        boundingRect.setBottomRight(
                    thumbnailLabels.at(thumbnailLabels.count()-1)->sceneBoundingRect().bottomRight());
    }
    return boundingRect;
}


void ThumbnailStrip::loadVisibleThumbnailsDelayed() {
    loadTimer.stop();
    loadTimer.start(LOAD_DELAY);
}


void ThumbnailStrip::loadVisibleThumbnails() {
    loadTimer.stop();
    updateVisibleRegion();

    for(int i=0; i<thumbnailLabels.count(); i++) {
        requestThumbnailLoad(i);
    }
}

void ThumbnailStrip::requestThumbnailLoad(int pos) {
    if(thumbnailLabels.at(pos)->state == EMPTY  && childVisible(pos)) {
        thumbnailLabels.at(pos)->state = LOADING;
        emit thumbnailRequested(pos);
    }
}

void ThumbnailStrip::addItem() {
    ThumbnailLabel *thumbLabel = new ThumbnailLabel();
    thumbLabel->setOpacityAnimated(0.0, ANIMATION_SPEED_INSTANT);
    thumbnailLabels.append(thumbLabel);
    layout->addItem(thumbLabel);
    requestThumbnailLoad(thumbnailLabels.length()-1);
}

void ThumbnailStrip::setThumbnail(int pos, const Thumbnail* thumb) {
    thumbnailLabels.at(pos)->setThumbnail(thumb);
    thumbnailLabels.at(pos)->state = LOADED;
    if(pos != current) {
        thumbnailLabels.at(pos)->setOpacityAnimated(OPACITY_INACTIVE, ANIMATION_SPEED_NORMAL);
    }
}

void ThumbnailStrip::updateVisibleRegion() {
    QRect viewport_rect(0, 0, width(), height());
    visibleRegion = mapToScene(viewport_rect).boundingRect();
    visibleRegion.adjust(-OFFSCREEN_PRELOAD_AREA,0,OFFSCREEN_PRELOAD_AREA,0);
}

bool ThumbnailStrip::childVisible(int pos) {
    if(thumbnailLabels.count() >pos) {
        return thumbnailLabels.at(pos)->
                sceneBoundingRect().intersects(visibleRegion);
    } else {
        return false;
    }
}

bool ThumbnailStrip::childVisibleEntirely(int pos) {
    if(thumbnailLabels.count() >pos) {
        QRectF visibleRegionReduced = visibleRegion.adjusted(500,0,-500,0);
        return thumbnailLabels.at(pos)->
                sceneBoundingRect().intersects(visibleRegionReduced);
    } else {
        return false;
    }
}

void ThumbnailStrip::sceneClicked(QPointF pos) {
    ThumbnailLabel *item = (ThumbnailLabel*)scene->itemAt(pos, QGraphicsView::transform());
    int itemPos = thumbnailLabels.indexOf(item);
    if(itemPos!=-1) {
        emit thumbnailClicked(itemPos);
    }
}

void ThumbnailStrip::wheelEvent(QWheelEvent *event) {
    event->setAccepted(true);

    if(timeLine->state() == QTimeLine::Running) {
        timeLine->stop();
        horizontalScrollBar()->setValue(timeLine->endFrame());
    }
    timeLine->setFrameRange(horizontalScrollBar()->value(),
                            horizontalScrollBar()->value()-event->angleDelta().ry()*2.5);
    timeLine->setUpdateInterval(16);
    timeLine->start();


    /* instant
    horizontalScrollBar()->setValue(horizontalScrollBar()->
                                    value()-event->angleDelta().ry());
    */

    /*
    QPointF viewCenter = mapToScene(width() / 2, 0);
    if(event->angleDelta().ry() < 0) {
        viewCenter += QPointF(SCROLL_STEP, 0);
    }
    else {
        viewCenter -= QPointF(SCROLL_STEP, 0);
    }
    centerOnSmooth(viewCenter);
    */
}

void ThumbnailStrip::parentResized(QSize parentSize) {
    this->setGeometry( QRect(0, parentSize.height() - panelHeight + 1,
                             parentSize.width(), panelHeight) );
    loadVisibleThumbnailsDelayed();
}

void ThumbnailStrip::leaveEvent(QEvent *event) {
    Q_UNUSED(event)
    hide();
}

ThumbnailStrip::~ThumbnailStrip() {

}