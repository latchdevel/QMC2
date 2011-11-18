#include <QPixmapCache>
#include <QPixmap>
#include <QImage>
#include <QMatrix>
#include <QByteArray>
#include <QClipboard>

#include "cabinet.h"
#include "options.h"
#include "gamelist.h"
#include "qmc2main.h"
#include "macros.h"

// external global variables
extern MainWindow *qmc2MainWindow;
extern Options *qmc2Options;
extern Gamelist *qmc2Gamelist;
extern bool qmc2UseCabinetFile;
extern bool qmc2GuiReady;
extern bool qmc2ReloadActive;
extern bool qmc2ScaledCabinet;
extern bool qmc2SmoothScaling;
extern bool qmc2RetryLoadingImages;
extern bool qmc2ParentImageFallback;
extern bool qmc2ShowGameName;
extern bool qmc2ShowGameNameOnlyWhenRequired;
extern QTreeWidgetItem *qmc2CurrentItem;
extern QSettings *qmc2Config;
extern QMap<QString, QString> qmc2ParentMap;
extern QMap<QString, QString> qmc2GamelistDescriptionMap;

Cabinet::Cabinet(QWidget *parent)
#if QMC2_OPENGL == 1
  : QGLWidget(parent)
#else
  : QWidget(parent)
#endif
{
#ifdef QMC2_DEBUG
  qmc2MainWindow->log(QMC2_LOG_FRONTEND, "DEBUG: Cabinet::Cabinet(QWidget *parent = 0x" + QString::number((ulong)parent, 16) + ")");
#endif

  contextMenu = new QMenu(this);
  contextMenu->hide();

  QString s;
  QAction *action;

  s = tr("Copy to clipboard");
  action = contextMenu->addAction(s);
  action->setToolTip(s); action->setStatusTip(s);
  action->setIcon(QIcon(QString::fromUtf8(":/data/img/editcopy.png")));
  connect(action, SIGNAL(triggered()), this, SLOT(copyToClipboard()));

#if defined(QMC2_EMUTYPE_MAME)
  setToolTip(tr("Game cabinet image"));
  setStatusTip(tr("Game cabinet image"));
#elif defined(QMC2_EMUTYPE_MESS)
  setToolTip(tr("Machine cabinet image"));
  setStatusTip(tr("Machine cabinet image"));
#endif

  cabinetFile = NULL;
  if ( qmc2UseCabinetFile ) {
#if defined(QMC2_EMUTYPE_MAME)
    cabinetFile = unzOpen((const char *)qmc2Config->value("MAME/FilesAndDirectories/CabinetFile").toString().toAscii());
    if ( cabinetFile == NULL )
      qmc2MainWindow->log(QMC2_LOG_FRONTEND, tr("FATAL: can't open cabinet file, please check access permissions for %1").arg(qmc2Config->value("MAME/FilesAndDirectories/CabinetFile").toString()));
#elif defined(QMC2_EMUTYPE_MESS)
    cabinetFile = unzOpen((const char *)qmc2Config->value("MESS/FilesAndDirectories/CabinetFile").toString().toAscii());
    if ( cabinetFile == NULL )
      qmc2MainWindow->log(QMC2_LOG_FRONTEND, tr("FATAL: can't open cabinet file, please check access permissions for %1").arg(qmc2Config->value("MESS/FilesAndDirectories/CabinetFile").toString()));
#endif
  }
}

Cabinet::~Cabinet()
{
#ifdef QMC2_DEBUG
  qmc2MainWindow->log(QMC2_LOG_FRONTEND, "DEBUG: Cabinet::~Cabinet()");
#endif

  if ( qmc2UseCabinetFile )
    unzClose(cabinetFile);
}

void Cabinet::paintEvent(QPaintEvent *e)
{
#ifdef QMC2_DEBUG
  qmc2MainWindow->log(QMC2_LOG_FRONTEND, "DEBUG: Cabinet::paintEvent(QPaintEvent *e = 0x" + QString::number((ulong)e, 16) + ")");
#endif

  QPainter p(this);

  if ( !qmc2CurrentItem ) {
    drawCenteredImage(0, &p); // clear cabinet widget
    return;
  }

  if ( qmc2CurrentItem->text(QMC2_GAMELIST_COLUMN_GAME) == tr("Waiting for data...") ) {
    drawCenteredImage(0, &p); // clear cabinet widget
    return;
  }

  QTreeWidgetItem *topLevelItem = qmc2CurrentItem;
  while ( topLevelItem->parent() )
    topLevelItem = topLevelItem->parent();

  QString gameName = topLevelItem->child(0)->text(QMC2_GAMELIST_COLUMN_ICON);

  if ( !QPixmapCache::find("cab_" + gameName, &currentCabinetPixmap) ) {
    qmc2CurrentItem = topLevelItem;
    loadCabinet(gameName, gameName);
  }
  if ( qmc2ScaledCabinet )
    drawScaledImage(&currentCabinetPixmap, &p);
  else
    drawCenteredImage(&currentCabinetPixmap, &p);
}

bool Cabinet::loadCabinet(QString gameName, QString onBehalfOf, bool checkOnly, QString *fileName)
{
#ifdef QMC2_DEBUG
  qmc2MainWindow->log(QMC2_LOG_FRONTEND, QString("DEBUG: Cabinet::loadCabinet(QString gameName = %1, QString onBehalfOf = %2, bool checkOnly = %3, QString *fileName = %4)").arg(gameName).arg(onBehalfOf).arg(checkOnly).arg((qulonglong)fileName));
#endif

  QPixmap pm;
  char imageBuffer[QMC2_ZIP_BUFFER_SIZE];

  if ( fileName )
    *fileName = "";

  bool fileOk = TRUE;

  if ( qmc2UseCabinetFile ) {
    // use cabinet file
    QByteArray imageData;
    int len, i;
    QString gameFile = gameName + ".png";

    if ( fileName )
      *fileName = gameFile;

    if ( unzLocateFile(cabinetFile, (const char *)gameFile.toAscii(), 0) == UNZ_OK ) {
      if ( unzOpenCurrentFile(cabinetFile) == UNZ_OK ) {
        while ( (len = unzReadCurrentFile(cabinetFile, &imageBuffer, QMC2_ZIP_BUFFER_SIZE)) > 0 ) {
          for (i = 0; i < len; i++)
            imageData += imageBuffer[i];
        }
        unzCloseCurrentFile(cabinetFile);
      } else
        fileOk = FALSE;
    } else
      fileOk = FALSE;

    if ( fileOk )
      fileOk = pm.loadFromData(imageData, "PNG");

    if ( !checkOnly ) {
      if ( fileOk ) {
        QPixmapCache::insert("cab_" + onBehalfOf, pm); 
        currentCabinetPixmap = pm;
      } else {
        QString parentName = qmc2ParentMap[gameName];
        if ( qmc2ParentImageFallback && !parentName.isEmpty() ) {
          loadCabinet(parentName, onBehalfOf);
        } else {
          if ( !qmc2RetryLoadingImages )
            QPixmapCache::insert("cab_" + onBehalfOf, qmc2MainWindow->qmc2GhostImagePixmap);
          currentCabinetPixmap = qmc2MainWindow->qmc2GhostImagePixmap;
        }
      }
    }
  } else {
    // use cabinet directory
#if defined(QMC2_EMUTYPE_MAME)
    QString imagePath = qmc2Config->value("MAME/FilesAndDirectories/CabinetDirectory").toString() + gameName + ".png";
#elif defined(QMC2_EMUTYPE_MESS)
    QString imagePath = qmc2Config->value("MESS/FilesAndDirectories/CabinetDirectory").toString() + gameName + ".png";
#endif

    if ( fileName )
      *fileName = gameName + ".png";

    if ( checkOnly ) {
      fileOk = pm.load(imagePath, "PNG");
    } else {
      if ( pm.load(imagePath, "PNG") ) {
        QPixmapCache::insert("cab_" + onBehalfOf, pm); 
        currentCabinetPixmap = pm;
        fileOk = TRUE;
      } else {
        QString parentName = qmc2ParentMap[gameName];
        if ( qmc2ParentImageFallback && !parentName.isEmpty() ) {
          loadCabinet(parentName, onBehalfOf);
        } else {
          if ( !qmc2RetryLoadingImages )
            QPixmapCache::insert("cab_" + onBehalfOf, qmc2MainWindow->qmc2GhostImagePixmap);
          currentCabinetPixmap = qmc2MainWindow->qmc2GhostImagePixmap;
          fileOk = FALSE;
        }
      }
    }
  }

  return fileOk;
}

void Cabinet::drawCenteredImage(QPixmap *pm, QPainter *p)
{
#ifdef QMC2_DEBUG
  qmc2MainWindow->log(QMC2_LOG_FRONTEND, "DEBUG: Cabinet::drawCenteredImage(QPixmap *pm = 0x" + QString::number((ulong)pm, 16) + ", QPainter *p = 0x" + QString::number((ulong)p, 16) + ")");
#endif

  p->eraseRect(rect());

  if ( pm == NULL ) {
    p->end();
    return;
  }

  // last resort if pm->load() retrieved a null pixmap...
  if ( pm->isNull() )
    pm = &qmc2MainWindow->qmc2GhostImagePixmap;

  int posx = (rect().width() - pm->width()) / 2;
  int posy = (rect().height() - pm->height()) / 2;

  p->drawPixmap(posx, posy, *pm);

  bool drawGameName = FALSE;
  if ( qmc2ShowGameName ) {
    if ( qmc2ShowGameNameOnlyWhenRequired ) {
      if ( qmc2MainWindow->hSplitter->sizes()[0] == 0 || qmc2MainWindow->tabWidgetGamelist->currentIndex() != QMC2_GAMELIST_INDEX ) {
        drawGameName = TRUE;
      } else {
        drawGameName = FALSE;
      }
    } else {
      drawGameName = TRUE;
    }
  } else
    drawGameName = FALSE;

  if ( drawGameName ) {
    // draw game/machine title
    QString title = QString("%1").arg(qmc2GamelistDescriptionMap[qmc2CurrentItem->child(0)->text(QMC2_GAMELIST_COLUMN_ICON)]);
    QFont f(qApp->font());
    f.setWeight(QFont::Bold);
    p->setFont(f);
    QFontMetrics fm(f);
    QRect r = rect();
    int adjustment = fm.height() / 2;
    r = r.adjusted(+adjustment, +adjustment, -adjustment, -adjustment);
    QRect outerRect = p->boundingRect(r, Qt::AlignCenter | Qt::TextWordWrap, title);
    r.setTop(r.bottom() - outerRect.height());
    r = p->boundingRect(r, Qt::AlignCenter | Qt::TextWordWrap, title);
    r = r.adjusted(-adjustment, -adjustment, +adjustment, +adjustment);
    r.setBottom(rect().bottom());
    p->setPen(QPen(QColor(255, 255, 255, 0)));
    p->fillRect(r, QBrush(QColor(0, 0, 0, 128), Qt::SolidPattern));
    p->setPen(QPen(QColor(255, 255, 255, 255)));
    p->drawText(r, Qt::AlignCenter | Qt::TextWordWrap, QString("%1").arg(qmc2GamelistDescriptionMap[qmc2CurrentItem->child(0)->text(QMC2_GAMELIST_COLUMN_ICON)]));
  }

  p->end();
}

void Cabinet::drawScaledImage(QPixmap *pm, QPainter *p)
{
#ifdef QMC2_DEBUG
  qmc2MainWindow->log(QMC2_LOG_FRONTEND, "DEBUG: Cabinet::drawScaledImage(QPixmap *pm = 0x" + QString::number((ulong)pm, 16) + ", QPainter *p = 0x" + QString::number((ulong)p, 16) + ")");
#endif

  if ( pm == NULL ) {
    p->eraseRect(rect());
    p->end();
    return;
  }

  // last resort if pm->load() retrieved a null pixmap...
  if ( pm->isNull() )
    pm = &qmc2MainWindow->qmc2GhostImagePixmap;

  double desired_width;
  double desired_height;

  if ( pm->width() > pm->height() ) {
    desired_width  = contentsRect().width();
    desired_height = (double)pm->height() * (desired_width / (double)pm->width());
    if ( desired_height > contentsRect().height() ) {
      desired_height = contentsRect().height();
      desired_width  = (double)pm->width() * (desired_height / (double)pm->height());
    }
  } else {
    desired_height = contentsRect().height();
    desired_width  = (double)pm->width() * (desired_height / (double)pm->height());
    if ( desired_width > contentsRect().width() ) {
      desired_width = contentsRect().width();
      desired_height = (double)pm->height() * (desired_width / (double)pm->width());
    }
  }

  QPixmap pmScaled;

  if ( qmc2SmoothScaling )
    pmScaled = pm->scaled((int)desired_width, (int)desired_height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
  else
    pmScaled = pm->scaled((int)desired_width, (int)desired_height, Qt::KeepAspectRatio, Qt::FastTransformation);

  drawCenteredImage(&pmScaled, p);
}

void Cabinet::copyToClipboard()
{
#ifdef QMC2_DEBUG
  qmc2MainWindow->log(QMC2_LOG_FRONTEND, "DEBUG: Cabinet::copyToClipboard()");
#endif

  qApp->clipboard()->setPixmap(currentCabinetPixmap);
}

void Cabinet::contextMenuEvent(QContextMenuEvent *e)
{
#ifdef QMC2_DEBUG
  qmc2MainWindow->log(QMC2_LOG_FRONTEND, QString("DEBUG: Cabinet::contextMenuEvent(QContextMenuEvent *e = %1)").arg((qulonglong)e));
#endif

  contextMenu->move(qmc2MainWindow->adjustedWidgetPosition(mapToGlobal(e->pos()), contextMenu));
  contextMenu->show();
}
