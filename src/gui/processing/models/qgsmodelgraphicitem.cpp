/***************************************************************************
                             qgsmodelgraphicitem.cpp
                             ----------------------------------
    Date                 : February 2020
    Copyright            : (C) 2020 Nyall Dawson
    Email                : nyall dot dawson at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsmodelgraphicitem.h"
#include "qgsmodelcomponentgraphicitem.h"
#include "moc_qgsmodelgraphicitem.cpp"
#include "qgsapplication.h"
#include "qgsmodelgraphicsscene.h"
#include "qgsmodelgraphicsview.h"
#include "qgsmodelviewtool.h"
#include "qgsmodelviewmouseevent.h"
#include "qgsprocessingmodelcomponent.h"
#include "qgsprocessingoutputs.h"
#include "qgsprocessingparameters.h"
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QSvgRenderer>

///@cond NOT_STABLE

QgsModelDesignerFlatButtonGraphicItem::QgsModelDesignerFlatButtonGraphicItem( QGraphicsItem *parent, const QPicture &picture, const QPointF &position, const QSizeF &size )
  : QGraphicsObject( parent )
  , mPicture( picture )
  , mPosition( position )
  , mSize( size )
{
  setAcceptHoverEvents( true );
  setFlag( QGraphicsItem::ItemIsMovable, false );
  setCacheMode( QGraphicsItem::DeviceCoordinateCache );
}

void QgsModelDesignerFlatButtonGraphicItem::paint( QPainter *painter, const QStyleOptionGraphicsItem *, QWidget * )
{
  if ( QgsModelGraphicsScene *modelScene = qobject_cast<QgsModelGraphicsScene *>( scene() ) )
  {
    if ( modelScene->flags() & QgsModelGraphicsScene::FlagHideControls )
      return;
  }

  if ( mHoverState )
  {
    painter->setPen( QPen( Qt::transparent, 1.0 ) );
    painter->setBrush( QBrush( QColor( 55, 55, 55, 33 ), Qt::SolidPattern ) );
  }
  else
  {
    painter->setPen( QPen( Qt::transparent, 1.0 ) );
    painter->setBrush( QBrush( Qt::transparent, Qt::SolidPattern ) );
  }
  const QPointF topLeft = mPosition - QPointF( std::floor( mSize.width() / 2 ), std::floor( mSize.height() / 2 ) );
  const QRectF rect = QRectF( topLeft.x(), topLeft.y(), mSize.width(), mSize.height() );
  painter->drawRect( rect );
  painter->drawPicture( topLeft.x(), topLeft.y(), mPicture );
}

QRectF QgsModelDesignerFlatButtonGraphicItem::boundingRect() const
{
  return QRectF( mPosition.x() - std::floor( mSize.width() / 2 ), mPosition.y() - std::floor( mSize.height() / 2 ), mSize.width(), mSize.height() );
}

void QgsModelDesignerFlatButtonGraphicItem::hoverEnterEvent( QGraphicsSceneHoverEvent * )
{
  if ( view()->tool() && !view()->tool()->allowItemInteraction() )
    mHoverState = false;
  else
    mHoverState = true;
  update();
}

void QgsModelDesignerFlatButtonGraphicItem::hoverLeaveEvent( QGraphicsSceneHoverEvent * )
{
  mHoverState = false;
  update();
}

void QgsModelDesignerFlatButtonGraphicItem::mousePressEvent( QGraphicsSceneMouseEvent * )
{
  if ( view()->tool() && view()->tool()->allowItemInteraction() )
    emit clicked();
}

void QgsModelDesignerFlatButtonGraphicItem::modelHoverEnterEvent( QgsModelViewMouseEvent * )
{
  if ( view()->tool() && !view()->tool()->allowItemInteraction() )
    mHoverState = false;
  else
    mHoverState = true;
  update();
}

void QgsModelDesignerFlatButtonGraphicItem::modelHoverLeaveEvent( QgsModelViewMouseEvent * )
{
  mHoverState = false;
  update();
}

void QgsModelDesignerFlatButtonGraphicItem::modelPressEvent( QgsModelViewMouseEvent *event )
{
  if ( view()->tool() && view()->tool()->allowItemInteraction() && event->button() == Qt::LeftButton )
  {
    QMetaObject::invokeMethod( this, "clicked", Qt::QueuedConnection );
    mHoverState = false;
    update();
  }
}

void QgsModelDesignerFlatButtonGraphicItem::setPosition( const QPointF &position )
{
  mPosition = position;
  prepareGeometryChange();
  update();
}

QgsModelGraphicsView *QgsModelDesignerFlatButtonGraphicItem::view()
{
  return qobject_cast<QgsModelGraphicsView *>( scene()->views().first() );
}

void QgsModelDesignerFlatButtonGraphicItem::setPicture( const QPicture &picture )
{
  mPicture = picture;
  update();
}

//
// QgsModelDesignerFoldButtonGraphicItem
//

QgsModelDesignerFoldButtonGraphicItem::QgsModelDesignerFoldButtonGraphicItem( QGraphicsItem *parent, bool folded, const QPointF &position, const QSizeF &size )
  : QgsModelDesignerFlatButtonGraphicItem( parent, QPicture(), position, size )
  , mFolded( folded )
{
  QSvgRenderer svg( QgsApplication::iconPath( QStringLiteral( "mIconModelerExpand.svg" ) ) );
  QPainter painter( &mPlusPicture );
  svg.render( &painter );
  painter.end();

  QSvgRenderer svg2( QgsApplication::iconPath( QStringLiteral( "mIconModelerCollapse.svg" ) ) );
  painter.begin( &mMinusPicture );
  svg2.render( &painter );
  painter.end();

  setPicture( mFolded ? mPlusPicture : mMinusPicture );
}

void QgsModelDesignerFoldButtonGraphicItem::mousePressEvent( QGraphicsSceneMouseEvent *event )
{
  mFolded = !mFolded;
  setPicture( mFolded ? mPlusPicture : mMinusPicture );
  emit folded( mFolded );
  QgsModelDesignerFlatButtonGraphicItem::mousePressEvent( event );
}

void QgsModelDesignerFoldButtonGraphicItem::modelPressEvent( QgsModelViewMouseEvent *event )
{
  mFolded = !mFolded;
  setPicture( mFolded ? mPlusPicture : mMinusPicture );
  emit folded( mFolded );
  QgsModelDesignerFlatButtonGraphicItem::modelPressEvent( event );
}


QgsModelDesignerSocketGraphicItem::QgsModelDesignerSocketGraphicItem( QgsModelComponentGraphicItem *parent, QgsProcessingModelComponent *component, int index, const QPointF &position, Qt::Edge edge, const QSizeF &size )
  : QgsModelDesignerFlatButtonGraphicItem( parent, QPicture(), position, size )
  , mComponentItem( parent )
  , mComponent( component )
  , mIndex( index )
  , mEdge( edge )
{
}

void QgsModelDesignerSocketGraphicItem::paint( QPainter *painter, const QStyleOptionGraphicsItem *, QWidget * )
{

  QColor outlineColor = getColor();
  QColor fillColor = QColor(outlineColor);
  fillColor.setAlpha(mHoverState ? 255 : 50 );




  // Outline style
  painter->setPen(QPen(outlineColor, mSocketOutlineWidth));

  // Fill style
  painter->setBrush( QBrush( fillColor, Qt::SolidPattern ) );

  painter->setRenderHint( QPainter::Antialiasing );

  constexpr float DISPLAY_SIZE = 4;
  painter->drawEllipse( getPosition(), DISPLAY_SIZE, DISPLAY_SIZE );
  /* Uncomment to display bounding box */
#if 0
  painter->save();
  painter->setPen( QPen() );
  painter->setBrush( QBrush() );
  painter->drawRect( boundingRect() );
  painter->restore();
#endif
}


QColor QgsModelDesignerSocketGraphicItem::getColor() {
  QString paramDataType = mComponentItem->getLinkedParamDataType(mEdge, mIndex);
  qDebug() << "______________ paramDataType: " << paramDataType;

  // Numerical types
  if(
      paramDataType == QgsProcessingParameterMatrix::typeName() ||
      paramDataType == QgsProcessingParameterNumber::typeName() ||
      paramDataType == QgsProcessingParameterRange::typeName() ||
      paramDataType == QgsProcessingParameterColor::typeName() ||
      paramDataType == QgsProcessingOutputNumber::typeName() ||
      paramDataType == QgsProcessingParameterDistance::typeName() ||
      paramDataType == QgsProcessingParameterDuration::typeName() ||
      paramDataType == QgsProcessingParameterScale::typeName()

    ) {
    return QColor(34, 157, 214);
  } else

  // Boolean type
  if(
    paramDataType == QgsProcessingParameterBoolean::typeName() ||
    paramDataType == QgsProcessingOutputBoolean::typeName()
  ) {
    return QColor(51, 201, 28);
  } else


  // Vector types
  if(
      paramDataType == QgsProcessingParameterPoint::typeName() ||
      paramDataType == QgsProcessingParameterGeometry::typeName() ||
      paramDataType == QgsProcessingParameterVectorLayer::typeName() ||
      paramDataType == QgsProcessingParameterMeshLayer::typeName() ||
      paramDataType == QgsProcessingParameterPointCloudLayer::typeName() ||
      paramDataType == QgsProcessingOutputVectorLayer::typeName() ||
      paramDataType == QgsProcessingOutputPointCloudLayer::typeName() ||
      paramDataType == QgsProcessingParameterExtent::typeName() ||
      paramDataType == QgsProcessingOutputVectorTileLayer::typeName() ||
      paramDataType == QgsProcessingParameterPointCloudDestination::typeName() ||
      paramDataType == QgsProcessingParameterVectorTileDestination::typeName() ||
      paramDataType == QgsProcessingParameterVectorDestination::typeName() ||
      paramDataType == QgsProcessingParameterFeatureSource::typeName()
  ) {
    return QColor(180, 180, 0);
  } else

  // Raster type
  if(
    paramDataType == QgsProcessingParameterRasterLayer::typeName() ||
    paramDataType == QgsProcessingOutputRasterLayer::typeName()

  ) {
    return QColor(0, 180, 180);
  } else

  // enum
  if(
    paramDataType == QgsProcessingParameterEnum::typeName()
  ) {
    return QColor(128, 68, 201);
  } else

  // String and datetime types
  if(
    paramDataType == QgsProcessingParameterString::typeName() ||
    paramDataType == QgsProcessingParameterDateTime::typeName() ||
    paramDataType == QgsProcessingParameterCrs::typeName() ||
    paramDataType == QgsProcessingOutputHtml::typeName() ||
    paramDataType == QgsProcessingOutputString::typeName()

  ) {
    return QColor(100, 100, 255);
  } else

  // filesystem types
  if(
    paramDataType == QgsProcessingParameterFile::typeName() ||
    paramDataType == QgsProcessingOutputFolder::typeName() ||
    paramDataType == QgsProcessingOutputFile::typeName() ||
    paramDataType == QgsProcessingParameterFolderDestination::typeName() ||
    paramDataType == QgsProcessingParameterFeatureSink::typeName() ||
    paramDataType == QgsProcessingParameterRasterDestination::typeName() ||
    paramDataType == QgsProcessingParameterFileDestination::typeName()
  ) {
    return QColor(80, 80, 80);
  } else

  // Expression type
  if(paramDataType == QgsProcessingParameterExpression::typeName()) {
    return QColor(180, 80, 180);
  } else

  // Other Layer types
  if(
    paramDataType == QgsProcessingParameterMultipleLayers::typeName() ||
    paramDataType == QgsProcessingParameterMapLayer::typeName() ||
    paramDataType == QgsProcessingParameterAnnotationLayer::typeName() ||
    paramDataType == QgsProcessingOutputMultipleLayers::typeName()

  ) {
    return QColor(128, 128, 0);
  } else

  // Default color, applies for:
  // QgsProcessingParameterField
  // QgsProcessingParameterMapTheme
  // QgsProcessingParameterBand
  // QgsProcessingParameterLayout
  // QgsProcessingParameterLayoutItem
  // QgsProcessingParameterCoordinateOperation
  // QgsProcessingParameterAuthConfig // config
  // QgsProcessingParameterDatabaseSchema
  // QgsProcessingParameterDatabaseTable
  // QgsProcessingParameterProviderConnection
  // QgsProcessingParameterPointCloudAttribute
  // QgsProcessingOutputVariant
  // QgsProcessingOutputConditionalBranch
  {
    return QColor(128, 128, 128);
  }


  /*


  */

}

///@endcond
