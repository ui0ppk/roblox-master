
#include "stdafx.h"

#include "SplineEditor.h"


#include <QLineEdit>
#include <QRadioButton>
#include <QTextDocument>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QKeyEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QTextBlock>
#include <QPainter>

#include "RobloxMainWindow.h"
#include "UpdateUIManager.h"
#include "v8datamodel/ChangeHistory.h"

#include "G3D/Vector4.h"
#include "G3d/g3dmath.h"
using G3D::Vector4;
using G3D::clamp;

#include "v8datamodel/CustomParticleEmitter.h"

static bool dbgmm = false; // debug mousemoves ?

struct Sortpr{ bool operator()(const Vector4& a, const Vector4& b ) const 
{
    return a.x < b.x;
}};

struct Killpr
{ 
    int id;
    bool operator()(const Vector4& a) const { return int(a.w + 0.5f) == id; }
};



SplineEditor::SplineEditor(QWidget* parent, SplineDef* def, const char* settingsName) : Base(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::FramelessWindowHint)
{
    RBXASSERT(def);

    m_sel = -1;
    m_selEnvHandle = -1;
    m_mode = Free;
    m_clickX = m_clickY = -100;
    m_keepHighlight = false;
    m_Xconstraint = -1;
    m_savePt.set(-1,-1,-1,-1);
    m_spline = def;

    if (m_spline->pt.empty())
    {
        m_spline->pt.push_back( Vector4(0,0,0,0) );
    }

    m_splineBackup  = new SplineDef(*m_spline);
    m_lastid        = m_spline->pt.size()+1000;

    m_pRootFrame    = new QFrame(this);
    m_pCanvasWidget = new SplineEditorCanvas(m_pRootFrame, this, m_spline);
    m_pResetButton  = new QPushButton("Reset", m_pRootFrame);
    m_pCloseButton  = new QPushButton("Close", m_pRootFrame);
    m_pXEdit        = new QLineEdit("-", m_pRootFrame);
    m_pYEdit        = new QLineEdit("-", m_pRootFrame);
    m_pEnvEdit      = new QLineEdit("-", m_pRootFrame);
    m_pKillPtButton = new QPushButton("Delete", m_pRootFrame);
    

    QGridLayout* xyBlock = new QGridLayout; // left side:    "Time"[X] | "Value"[Y] | "Envelope"[E]    
    xyBlock->addWidget(new QLabel("Time"),     0, 0);
    xyBlock->addWidget(m_pXEdit,               0, 1);
    xyBlock->addItem(new QSpacerItem(30, 0),   0, 2);
    xyBlock->addWidget(new QLabel("Value"),    0, 3);
    xyBlock->addWidget(m_pYEdit,               0, 4);
    xyBlock->addItem(new QSpacerItem(30, 0),   0, 5);
    xyBlock->addWidget(new QLabel("Envelope"), 0, 6);
    xyBlock->addWidget(m_pEnvEdit,             0, 7);
    xyBlock->addWidget(m_pKillPtButton,        0, 8, Qt::AlignTop);
    
    // right side: Reset and Close buttons
    QGridLayout* buttonBlock = new QGridLayout;
    buttonBlock->addWidget(m_pResetButton, 0, 0 );
    buttonBlock->addWidget(m_pCloseButton, 0, 1 );

    // Bottom row: XYblock | empty space | button block
    QGridLayout* bottomRow = new QGridLayout;
    bottomRow->addLayout( xyBlock, 0, 0, Qt::AlignLeft );
    bottomRow->addItem(new QSpacerItem(100, 0), 0, 1);
    bottomRow->addLayout( buttonBlock, 0, 2, Qt::AlignRight );
    bottomRow->setColumnStretch(0, 0 );
    bottomRow->setColumnStretch(1, 10 );
    bottomRow->setColumnStretch(2, 0);
    
    // main layout
    QGridLayout* toplevel = new QGridLayout;

    m_pCanvasWidget->setMinimumSize( QSize(200,200) );
    toplevel->addWidget( m_pCanvasWidget, 0, 0, 1, 1 );
    toplevel->addLayout( bottomRow, 1, 0, 1, 1 );

    m_pRootFrame->setLayout( toplevel );
    
    this->setWidget(m_pRootFrame);
    this->setObjectName("SplineEditor");

    connect( m_pXEdit, SIGNAL(textEdited(QString)), this, SLOT(onPointDataChanged()) );
    connect( m_pYEdit, SIGNAL(textEdited(QString)), this, SLOT(onPointDataChanged()) );
    connect( m_pEnvEdit, SIGNAL(textEdited(QString)), this, SLOT(onPointDataChanged()) );
    connect( m_pKillPtButton, SIGNAL(clicked()), this, SLOT(onKillPtClicked()) );

    connect( m_pResetButton, SIGNAL(clicked()), this, SLOT(onReset()) );
    connect( m_pCloseButton, SIGNAL(clicked()), this, SLOT(onClose()) );
    

    m_settingsName = QString("SplineEditor/") + settingsName;
    QSettings settings;
    if( !restoreGeometry(settings.value(m_settingsName  + "/geometry").toByteArray()) )
        this->setFloating(true);
    this->setFloating( !settings.value(m_settingsName + "/docked").toBool() );

    select( -1 );
    this->setFocusPolicy(Qt::StrongFocus);
}

SplineEditor::~SplineEditor()
{
    QSettings settings;
    settings.setValue(m_settingsName + "/geometry", saveGeometry());
    settings.setValue(m_settingsName + "/docked", !this->isFloating());
}


void SplineEditor::onCanvasMouse( QMouseEvent* e )
{
    Qt::MouseButtons buttons = e->buttons();

    switch( e->type() )
    {
    case QEvent::MouseButtonPress:
        {
            // depends on what we click and how...
            if( buttons == Qt::LeftButton && m_mode == Free)
            {
                Selection what = updateSelection( e->x(), e->y() ); 
                if( what == Sel_Point ) // we're on a point, begin dragging it
                {
                    if (dbgmm) qDebug("MousePress left click - on a point handle");

                    m_clickX = e->x();
                    m_clickY = e->y();

                    int idx = pointById( getsel() );
                    RBXASSERT( idx >= 0 );
                    m_savePt = m_spline->pt[idx];

                    if( getsel() == leftmostPoint() ) m_Xconstraint = 0;
                    else if( getsel() == rightmostPoint() ) m_Xconstraint = 1;
                    else m_Xconstraint = -1;

                    m_pCanvasWidget->select(getsel());
                    m_pCanvasWidget->update();
                    m_mode = Drag;
                }
                else if( what == Sel_EnvUpper || what == Sel_EnvLower )
                {
                    if (dbgmm) qDebug("MousePress left click - on an envelope handle");

                    m_clickX = e->x();
                    m_clickY = e->y();
                    int idx = pointById( getsel() );
                    RBXASSERT( idx >= 0 );
                    m_savePt = m_spline->pt[idx];

                    m_pCanvasWidget->select(getsel());
                    m_pCanvasWidget->update();
                    m_selEnvHandle = what;
                    m_mode = DragEnvHandle;
                }
                else if( m_spline->pt.size() < RBX::NumberSequence::kMaxSize ) // we're not on a point, create a new point and drag it
                {
                    if (dbgmm) qDebug("MousePress left click - not on a point handle");
                    float ptx,pty;
                    m_pCanvasWidget->cvt( &ptx, &pty, e->x(), e->y() );
                    if( ptx < 0 || ptx > 1 || pty < 0 || pty > m_spline->ymax ) 
                        break; // false alarm

                    Vector4* pt = newPoint( ptx, pty, -1 );
                    select( pt->w + 0.5f );
                    sort();
                    pt = &m_spline->pt[ pointById(m_sel) ]; // re-acquire it
                    constrainValue(pt); // fit the value
                    
                    m_savePt.set(-1,-1,pt->z, -1); // save the initial envelope to pt->z, because dragging code uses that to constrain the envelopes
                    m_clickX = e->x();
                    m_clickY = e->y();
                    m_keepHighlight = true;

                    if( m_spline->pt.size() <= 2 ) // we've just created the second point
                    {
                        if( getsel() == leftmostPoint() ) pt->x = m_Xconstraint = 0; 
                        else if( getsel() == rightmostPoint() ) pt->x = m_Xconstraint = 1;
                        else m_Xconstraint = -1;
                    }
 
                    updateGhosting( -1, -1, false );
                    syncPoint(getsel(), true);
                    m_pCanvasWidget->select(getsel());
                    m_pCanvasWidget->update();
                    m_mode = DragNew;
                    Q_EMIT splineEdited(false);
                }
                break;
            }
            
            if( (buttons & Qt::RightButton && m_mode == Drag)  // the user right-clicks while dragging an existing point
                || (buttons & Qt::LeftButton && m_mode == DragEnv) // -or- the user left-clicks while dragging an envelope
                || (buttons & Qt::RightButton && m_mode == DragEnvHandle) ) // -or- the user right-clicks while dragging an envelope size handle
            {
                if (dbgmm) qDebug("MousePress left+right - while dragging");

                int idx = pointById( getsel() );
                RBXASSERT( idx >= 0 );
                m_spline->pt[idx] = m_savePt;

                m_savePt.set(-1, -1, -1, -1 );
                select( -1 );
                m_clickX = m_clickY = -100;
                m_keepHighlight = false;
                m_selEnvHandle = 0;

                sort();
                updateGhosting( e->x(), e->y(), true );
                m_pCanvasWidget->select(getsel());
                m_pCanvasWidget->update();
                m_mode = Free;
                Q_EMIT splineEdited(false);
                break;
            }

            if( buttons & Qt::RightButton && m_mode == DragNew ) // user right-clicks while drag-creating a new point
            {
                if (dbgmm) qDebug("MousePress right click - while newDragging");

                killPoint(getsel()); // abort creation

                m_savePt.set(-1, -1, -1, -1 );
                select(-1);
                m_clickX = m_clickY = -100;
                m_keepHighlight = false;

                sort();
                updateGhosting( e->x(), e->y(), true );
                m_pCanvasWidget->select(getsel());
                m_pCanvasWidget->update();
                m_mode = Free;
                Q_EMIT splineEdited(false);
                break;
            }

            
            if( buttons & Qt::RightButton && m_mode == Free ) // user right-drags a handle
            {
                bool b = updateSelection( e->x(), e->y() ); 
                if( b )
                {
                    qDebug("Mousepress right - on a handle");

                    m_clickX = e->x();
                    m_clickY = e->y();

                    int idx = pointById( getsel() );
                    RBXASSERT( idx >= 0 );
                    m_savePt = m_spline->pt[idx];
                    m_keepHighlight = true;

                    m_pCanvasWidget->select(getsel());
                    m_pCanvasWidget->update();
                    m_mode = DragEnv;
                }
                break;
            }

            
        }
        break;
    case QEvent::MouseButtonRelease:
        {
            switch( m_mode )
            {
            case Drag:
            case DragNew:
                {
                    if (dbgmm) qDebug( "MouseRelease - while dragging" );

                    int idx = pointById(getsel());
                    RBXASSERT(idx >= 0);
                    Vector4* pt = &m_spline->pt[idx];

                    bool createUndoPoint = true;

                    if( pt->x < 0 || pt->x > 1 )
                    {
                        killPoint(getsel()); // feature: dragging the point outside of the 0..1 range kills it
                        createUndoPoint = (m_mode == Drag); // this will count as 'delete existing'
                    }
                    else
                    {
                        constrainValue(pt); // fit the value
                    }
                    
                    m_savePt.set(-1, -1, -1, -1 );
                    m_clickX = m_clickY = -100;
                    m_keepHighlight = false;
                    m_Xconstraint = -1;

                    updateSelection( e->x(), e->y() );
                    updateGhosting( e->x(), e->y(), true );
                    m_mode = Free; // done moving the point
                    Q_EMIT splineEdited(createUndoPoint);
                    break;
                }
            case DragEnv:
            case DragEnvHandle:

                m_savePt.set(-1, -1, -1, -1 );
                m_clickX = m_clickY = -100;
                m_keepHighlight = false;
                m_Xconstraint = -1;

                updateSelection( e->x(), e->y() );
                updateGhosting( e->x(), e->y(), true );
                m_mode = Free; // done moving the point
                Q_EMIT splineEdited(true);
                break;
            default:
                break;
            }
            break;
        }
    case QEvent::MouseMove:
        {
            this->setFocus();
            updateGhosting( e->x(), e->y(), m_mode == Free );
            switch(m_mode)
            {
            case Free:
                if (dbgmm) qDebug( "MouseMove - free movement" );

                updateSelection( e->x(), e->y() );
                break;
            case Drag:
            case DragNew:
                {
                    if (dbgmm) qDebug( "MouseMove - while dragging" );

                    int idx = pointById(getsel());
                    RBXASSERT( idx >= 0 );
                    Vector4* pt = &m_spline->pt[ idx ];

                    int x = e->x(), y = e->y();
                    m_pCanvasWidget->cvt( &pt->x, &pt->y, x, y ); // move to the new location

                    if( m_Xconstraint >= 0 )
                        pt->x = m_Xconstraint;

                    constrainValue(pt); // fit the value

                    pt->z = m_savePt.z;
                    constrainEnvelope(pt); // fit the envelope

                    sort();
                    m_pCanvasWidget->update();
                    syncPoint(getsel(), true);
                    Q_EMIT splineEdited(false);
                    break;
                }
            case DragEnv:
                {
                    if (dbgmm) qDebug( "MouseMove - while right-dragging" );

                    int idx = pointById(getsel());
                    RBXASSERT( idx >= 0 );
                    Vector4* pt = &m_spline->pt[ idx ];

                    int x = e->x(), y = e->y();
                    float fx, fy, bx, by;

                    m_pCanvasWidget->cvt( &bx, &by, m_clickX, m_clickY ); // drag start point in spline space
                    m_pCanvasWidget->cvt( &fx, &fy, x, y ); // cursor position in spline space
                    
                    pt->z = ( fy - by + m_savePt.z ); // new envelope value
                    constrainEnvelope( pt );

                    m_pCanvasWidget->update();
                    syncPoint(getsel(), true);
                    Q_EMIT splineEdited(false);
                    break;
                }
            case DragEnvHandle:
                {
                    if (dbgmm) qDebug( "MouseMove - while dragging envelope handle");

                    int idx = pointById(getsel());
                    RBXASSERT( idx >= 0 );
                    Vector4* pt = &m_spline->pt[ idx ];

                    int x = e->x(), y = e->y();
                    float fx, fy, bx, by;

                    m_pCanvasWidget->cvt( &bx, &by, m_clickX, m_clickY );
                    m_pCanvasWidget->cvt( &fx, &fy, x, y );

                    switch(m_selEnvHandle)
                    {
                    case Sel_EnvUpper: pt->z = (fy - by + m_savePt.z); break;
                    case Sel_EnvLower: pt->z = (by - fy + m_savePt.z); break;
                    default: RBXASSERT(0); break;
                    }
                    
                    constrainEnvelope( pt );

                    m_pCanvasWidget->update();
                    syncPoint(getsel(), true);
                    Q_EMIT splineEdited(false);
                    break;
                }
            default:
                break;
            }
        }
        break;

    default:
        break;
    }
}

void SplineEditor::onCanvasLeave( QEvent* e )
{
    m_selEnvHandle = 0;
    updateGhosting(-1,-1,false);
    m_pCanvasWidget->update();
}

SplineEditor::Selection SplineEditor::updateSelection( int x, int y )
{
    int sel = m_pCanvasWidget->getPointAt( x, y );
    if (sel>=0 )
    {
        if( sel != getsel() )
        {
            select(sel);
            m_pCanvasWidget->select(sel);
            syncPoint(sel, true);
        }
        m_pKillPtButton->setEnabled( getsel() >= 0 && getsel() != leftmostPoint() && getsel() != rightmostPoint() );
        m_pCanvasWidget->setHighlight(1);
        m_pCanvasWidget->update();
        return Sel_Point;
    }

    std::pair<int,int> hsel = m_pCanvasWidget->getEnvHandleAt(x, y);
    if( hsel.first == getsel() && hsel.second >= 0 ) // we're pointing at the envelope handle of the currently selected point
    {
        m_pCanvasWidget->setHighlight( 2 + hsel.second );
        m_pCanvasWidget->update();
        return hsel.second == 0 ? Sel_EnvUpper : Sel_EnvLower;
    }

    // keep the selection but remove the highlight, unless told to keep it...
    m_pCanvasWidget->setHighlight( int(false || m_keepHighlight) );
    m_pCanvasWidget->update();

    return Sel_None;
}

void SplineEditor::syncPoint(int sel, bool direction)
{
    int idx = pointById(sel);
    if( idx<0 ) return;

    Vector4& v = m_spline->pt[idx];

    if( direction ) // write to widgets
    {
        m_pXEdit->setText( QString::number( v.x, 'g', 3 ) );
        m_pYEdit->setText( QString::number( v.y, 'g', 3 ) );
        m_pEnvEdit->setText( QString::number( v.z, 'g', 3 ) );
    }
    else // read from widgets
    {
        bool ok[3];
        float x = m_pXEdit->text().toFloat(ok);
        float y = m_pYEdit->text().toFloat(ok+1);
        float z = m_pEnvEdit->text().toFloat(ok+2);
        if( ok[0] && ok[1] && ok[2] )
        {
            v.x = clamp( x, 0, 1 ); 
            v.y = y; 
            v.z = z;

            constrainValue(&v);
            constrainEnvelope(&v);

            Q_EMIT splineEdited(true);
        }
    }
}

int SplineEditor::pointById(int id)
{
    for( int j=0, e=m_spline->pt.size(); j<e; ++j )
    {
        if( int(m_spline->pt[j].w + 0.5) == id )
        {
            return j;
        }
    }
    return -1;
}

Vector4* SplineEditor::newPoint(float x, float y, float e)
{
    if( e < 0 )
    {
        e = autoEnvelope(x, y);
    }
    m_spline->pt.push_back( Vector4(x, y, e, m_lastid++) );
    return &m_spline->pt.back();
}

void SplineEditor::killPoint(int id)
{
    int idx = this->pointById(id);
    if( idx < 0) 
        return;
    
    m_spline->pt.erase(m_spline->pt.begin() + idx);
}

float SplineEditor::autoEnvelope( float x, float y )
{
    std::vector< Vector4 >& pts = m_spline->pt;
    float ymax = m_spline->ymax;
    for( int j=1, e=pts.size(); j<e; ++j )
    {
        if( pts[j].x > x )
        {
            j -= 1;
            bool notLast = j + 1 < (int)pts.size();

            float x1 = pts[j].x;
            float x2 = notLast ? pts[j+1].x : x1+1;
            float e1 = pts[j].z;
            float e2 = notLast ? pts[j+1].z : e1;

            float s = (x - x1)/(x2 - x1);
            float e = fabsf( e1 + (e2-e1)*s );

            if( y + e > ymax )  e = ymax - y;
            if( y - e < 0)      e = y;
            if( e < 0 )         e = 0;
            return e;
        }
    }
    return 0;
}

void SplineEditor::constrainValue(G3D::Vector4* pt)
{
    float ymax = m_spline->ymax;
    if (pt->y < 0) pt->y = 0;
    if (pt->y > ymax) pt->y = ymax;
}

void SplineEditor::constrainEnvelope( Vector4* pt )
{
    float e = pt->z;
    float ymax = m_spline->ymax;
    if( e < 0 ) 
        e = 0;
    if( pt->y + e > ymax )  e = ymax - pt->y;
    if( pt->y - e < 0 )  e = pt->y;
    pt->z = e;
}

void SplineEditor::sort()
{
    std::sort( m_spline->pt.begin(), m_spline->pt.end(), Sortpr() ); // sort the points by x
}

int SplineEditor::leftmostPoint()
{
    if( m_spline->pt.empty() ) 
        return -1;
    return m_spline->pt.front().w + 0.5f;
}

int SplineEditor::rightmostPoint()
{
    if( m_spline->pt.empty() ) 
        return -1;
    return m_spline->pt.back().w + 0.5f;
}


void SplineEditor::onPointDataChanged()
{
    syncPoint(getsel(), false); // read
    m_pCanvasWidget->update();
}

void SplineEditor::updateGhosting(int cx, int cy, bool show)
{
    show = show && m_spline->pt.size() < RBX::NumberSequence::kMaxSize;
    if( show )
    {
        float x,y;
        m_pCanvasWidget->cvt( &x, &y, cx, cy );
        m_pCanvasWidget->ghost( x, y, 0 );
    }
    else
    {
        m_pCanvasWidget->ghost(-1,-1,-1);
    }
}

void SplineEditor::select( int id )
{
    m_sel = id;
    bool good = pointById(id) >= 0;

    m_pXEdit->setEnabled( good );
    m_pYEdit->setEnabled( good );
    m_pEnvEdit->setEnabled( good );
    
    if( !good )
    {
        m_pXEdit->setText("-");
        m_pYEdit->setText("-");
        m_pEnvEdit->setText("-");
    }
}

int SplineEditor::getsel()
{
    return m_sel;
}

void SplineEditor::onKillPtClicked()
{
    int pt = getsel();
    if (pt >= 0 && ( pt != leftmostPoint() && pt != rightmostPoint() ))
    {
        killPoint(pt);
        select(-1);
        m_pCanvasWidget->update();
        Q_EMIT splineEdited(true);
    }
}

void SplineEditor::onReset()
{
    select(-1);
    m_spline->pt = m_splineBackup->pt;
    Q_EMIT splineEdited(true);
    refreshData(0);
}

void SplineEditor::onClose()
{
    close();
}

void SplineEditor::closeEvent(QCloseEvent *event)
{
    Base::closeEvent(event);
    Q_EMIT finished();
}

void SplineEditor::refreshData(void* arg)
{
    if (dbgmm) qDebug("refreshData()");

    // abort drags
    switch(m_mode)
    {
    case Drag:
    case DragEnv:
        {
            int idx = pointById( getsel() );
            RBXASSERT( idx >= 0 );
            m_spline->pt[idx] = m_savePt;
            break;
        }
    case DragNew:
        {
            killPoint(getsel()); // abort creation
            break;
        }
    default: break;
    }

    if (arg)
    {
        SplineDef* sd = reinterpret_cast<SplineDef*>(arg);
        *m_spline = *sd;
        delete sd;
    }
    
    m_savePt.set(-1, -1, -1, -1 );
    select( -1 );
    m_clickX = m_clickY = -100;
    m_keepHighlight = false;
    m_mode = Free;
    m_selEnvHandle = 0;

    sort();
    updateGhosting( -1, -1, false );
    Base::update();
    m_pCanvasWidget->select(getsel());
    m_pCanvasWidget->update();

    if (dbgmm) qDebug("refreshData done!()");
}

void SplineEditor::keyPressEvent(QKeyEvent * event)
{
    if (event->key() == Qt::Key_Delete)
    {
        event->accept();

        int id = getsel();
        if (id < 0 || id == leftmostPoint() || id == rightmostPoint() )
            return

        killPoint(id);
        m_savePt.set(-1, -1, -1, -1 );
        select(-1);
        updateGhosting( -1, -1, false );
        m_clickX = m_clickY = -100;
        m_keepHighlight = false;
        m_pCanvasWidget->select(getsel());
        m_pCanvasWidget->update();
        m_mode = Free;

        Q_EMIT splineEdited(true);
    }
}

bool SplineEditor::event(QEvent* ev)
{
    if (ev->type() == QEvent::ShortcutOverride)
    {
        QKeyEvent* ke = static_cast<QKeyEvent*>(ev);
        if (ke->key() == Qt::Key_Delete)
        {
            ev->accept(); // this will inhibit the shortcut and keyPressEvent will be called
            return true;
        }
    }
    return Base::event(ev);
}

//////////////////////////////////////////////////////////////////////////

SplineEditorCanvas::SplineEditorCanvas(QWidget* parent, SplineEditor* owner, SplineDef* spline ) : QWidget(parent), m_pOwner(owner), m_spline(spline)
{
    m_selPt = 1;
    m_highlight_ = 0;
    setMouseTracking(true);

    m_ghostX = m_ghostY = m_ghostE = -1;
}

SplineEditorCanvas::~SplineEditorCanvas()
{

}

const int boxSize = 4;
const int gridH = 5; // number of horizontal grid lines
const int gridV = 11; // number of vertical grid lines
const int kSizeHandleLength = 20; //
const int kSizeHandleWidth  = 3;

void SplineEditorCanvas::paintEvent(QPaintEvent * e)
{
    int W = this->width(), H = this->height();
    
    (void)H;

    const std::vector< Vector4 >& src = m_spline->pt; // source points
    std::vector< QPoint > dest( src.size() ); // translated points
    std::vector< int > destEnv( src.size() );
    
    for( int j=0, e=dest.size(); j<e; ++j )
    {
        cvt( &dest[j].rx(), &dest[j].ry(), src[j].x, src[j].y ); // conversion

        int ex,ey;
        cvt( &ex, &ey, src[j].x, src[j].y + src[j].z ); // envelope
        destEnv[j] = abs(ey - dest[j].y());
    }

    if( src.size() == 1 )
    {
        QPoint v( W, dest.front().y() );
        dest.push_back( v );
        float e = destEnv[0];
        destEnv.push_back( e );
    }

    // drawing:
    // the order is roughly: frame, envelopes, grid lines, lines, point handles

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    QBrush blackBrush     = QBrush( QColor(20, 20, 20, 255) );
    QBrush ltgrayBrush    = QBrush( QColor(170, 170, 170, 255) );
    QBrush dkgrayBrush    = QBrush( QColor(90, 90, 90, 255) );
    QBrush redBrush       = QBrush( QColor(224, 100, 100, 255) );
    QBrush envelopeBrush  = QBrush( QColor(250, 190, 190, 255) );
    QBrush hightlightBrush = QBrush( QColor( 240, 240, 90, 255) );
    QBrush transparentBrush = QBrush( QColor( 0,0,0,0) );
    QPen   thinPen        = QPen( blackBrush, 1.0f );
    QPen   thinDashedPen  = QPen( blackBrush, 1.0f, Qt::DashLine );
    QPen   dottedPen      = QPen( ltgrayBrush, 1.0f, Qt::DotLine );
    QPen   ghostPen       = QPen( dkgrayBrush, 1.0f, Qt::DotLine );
    QPen   envelopePen    = QPen( envelopeBrush, 1.0f, Qt::SolidLine );
    QPen   envelopeHandlePen1 = QPen( blackBrush, 1.0f, Qt::SolidLine );
    QPen   envelopeHandlePen2 = QPen( blackBrush, float(kSizeHandleWidth), Qt::SolidLine );


    // frame
    //p.setPen( thinPen );
    //p.drawRect( QRect( QPoint(0,0), size() ) );

    // envelopes
    p.setBrush( envelopeBrush );
    p.setPen( envelopePen );
    for( int j=0, e = dest.size()-1; j<e; ++j )
    {
        QPointF poly[4] = {
            dest[j]   - QPointF( 0, destEnv[j] ),     // top left
            dest[j+1] - QPointF( 0, destEnv[j+1] ),   // top right
            dest[j+1] + QPointF( 0, destEnv[j+1] ),   // bottom right
            dest[j]   + QPointF( 0, destEnv[j] ),    // bottom left
        };
        p.drawPolygon( poly, 4 );
    }


    // grid lines
    // NOTE: do not draw these in spline space: the scaling will kill it
    p.setBrush( blackBrush );
    p.setPen( dottedPen );
    for( int j=0; j<gridV; ++j ) // vertical lines
    {
        float xf = float(j)/(gridV - 1);

        int x,y0,y1;
        cvt( &x, &y0, xf, 0.0f );
        cvt( &x, &y1, xf, m_spline->ymax );

        p.drawLine( x, y0, x, y1 );
    }

    for( int j=0; j<gridH; ++j )
    {
        float yf = m_spline->ymax * float(j) / (gridH - 1);

        int x0, x1, y;
        cvt( &x0, &y, 0.0f, yf );
        cvt( &x1, &y, 1.0f, yf );
        p.drawLine( x0, y, x1, y );
    }

    // ghost
    if( m_ghostX >= 0 && m_ghostX <= 1 && m_ghostY >= 0 && m_ghostY <= m_spline->ymax && !m_highlight_ )
    {
        // find the end points
        int idx = -1;
        for( int j=1, e=(int)src.size(); j<e; ++j )
        {
            if( m_ghostX < src[j].x )
            {
                idx = j-1;
                break;
            }
        }

        if( idx >= 0 )
        {
            int gx, gy;
            cvt( &gx, &gy, m_ghostX, m_ghostY );

            p.setPen( ghostPen );
            p.setBrush( transparentBrush );

            p.drawRect( gx - boxSize/2, gy - boxSize/2, boxSize, boxSize  );

            p.drawLine( dest[idx], QPoint(gx,gy) );
            p.drawLine( QPoint(gx,gy), dest[idx+1] );
        }
        else if( dest.size() > 0 )
        {
            int gx, gy;
            cvt( &gx, &gy, m_ghostX, m_ghostY );

            p.setPen( ghostPen );
            p.setBrush( transparentBrush );

            gx = W*(src[0].x < 0.5f);

            p.drawRect( gx - boxSize/2, gy - boxSize/2, boxSize, boxSize  );
            p.drawLine( dest[0], QPoint(gx ,gy) );
        }
    }

    // lines
    if( src.size() > 1 )
    {
        p.setPen( thinPen );
        p.drawPolyline( &dest[0], dest.size() );
        p.setBrush(blackBrush);
        p.setBackgroundMode( Qt::OpaqueMode );
    }
    else if( src.size() == 1 )
    {
        p.setPen( thinDashedPen );
        p.drawLine( QPoint( 0, dest[0].y() ), QPoint(W,dest[0].y()) );
    }

    

    // point handles
    p.setPen( thinPen );
    p.setBrush( blackBrush );
    for( int j=0, e=src.size(); j<e; ++j )
    {
        p.drawRect( dest[j].x() - boxSize/2, dest[j].y() - boxSize/2, boxSize, boxSize  );

        if (m_selPt == (int)(src[j].w + 0.5f) ) // by point id
        {
            p.setBrush( m_highlight_ == 1 ? hightlightBrush : redBrush );
            p.drawRect( dest[j].x() - boxSize, dest[j].y() - boxSize, 2*boxSize, 2*boxSize  );

            // envelope handles
            p.setPen( m_highlight_ == 2 ? envelopeHandlePen2 : envelopeHandlePen1 );
            p.drawLine( dest[j].x(), dest[j].y() - destEnv[j], dest[j].x(), dest[j].y() - destEnv[j] - kSizeHandleLength );

            p.setPen( m_highlight_ == 3 ? envelopeHandlePen2 : envelopeHandlePen1 );
            p.drawLine( dest[j].x(), dest[j].y() + destEnv[j], dest[j].x(), dest[j].y() + destEnv[j] + kSizeHandleLength );

            p.setPen(thinPen);
            p.setBrush( blackBrush );
        }
    }
}

int SplineEditorCanvas::getPointAt(int x, int y)
{
    const std::vector< Vector4 >& src = m_spline->pt; // source points

    for( int j=0, e=src.size(); j<e; ++j )
    {
        int ptx, pty;
        cvt( &ptx, &pty, src[j].x, src[j].y );
        if( abs( ptx - x) <= boxSize/2+3 && abs( pty - y ) <= boxSize/2+3 )
            return src[j].w + 0.5f;
    }
    return -1;
}


std::pair<int,int> SplineEditorCanvas::getEnvHandleAt(int x, int y)
{
    const std::vector< Vector4 >& src = m_spline->pt; // source points

    const int boxWidth  = kSizeHandleWidth + 7;
    const int boxHeight = kSizeHandleLength;

    for( int j=0, e=src.size(); j<e; ++j )
    {
        int ptid = src[j].w + 0.5f;
        int ptx, pty1, pty2, pte;
        cvt( &ptx, &pty1, src[j].x, src[j].y );
        cvt( &ptx, &pty2, src[j].x, src[j].y - src[j].z );
        pte = abs(pty1 - pty2); // envelope in pixels

        QRect upper( ptx - boxWidth/2, pty1 - pte - boxHeight, boxWidth, boxHeight );
        QRect lower( ptx - boxWidth/2, pty1 + pte,             boxWidth, boxHeight );
        
        if( upper.contains(x,y) ) 
            return std::make_pair(ptid, 0);
        if( lower.contains(x,y) ) 
            return std::make_pair(ptid, 1);
    }
    return std::make_pair(-1,-1);
}

static const int kHBorderSize = 5;
static const int kVBorderSize = 20;

void SplineEditorCanvas::cvt(float* fx, float* fy, int x, int y)
{
    float sx = 1.0f/(this->width()  - 2*kHBorderSize);
    float sy = 1.0f/(this->height() - 2*kVBorderSize);

    *fx = (x - kHBorderSize ) * sx;
    *fy = m_spline->ymax * (1.0f - (y - kVBorderSize)*sy);
}

void SplineEditorCanvas::cvt( int* ix, int* iy, float x, float y )
{
    float sx = this->width()  - 2*kHBorderSize;
    float sy = this->height() - 2*kVBorderSize;

    *ix = x * sx + kHBorderSize;
    *iy = ( 1.0f - y / m_spline->ymax ) * sy + kVBorderSize;
}

void SplineEditorCanvas::mouseMoveEvent(QMouseEvent * e)
{
    m_pOwner->onCanvasMouse(e);
}

void SplineEditorCanvas::mousePressEvent(QMouseEvent * e)
{
    m_pOwner->onCanvasMouse(e);
}

void SplineEditorCanvas::mouseReleaseEvent(QMouseEvent * e)
{
    m_pOwner->onCanvasMouse(e);
}

void SplineEditorCanvas::leaveEvent( QEvent* e )
{
    m_pOwner->onCanvasLeave(e);
}


//////////////////////////////////////////////////////////////////////////

#include "reflection/Property.h"
#include "v8datamodel/DataModel.h"
#include "v8datamodel/NumberSequence.h"

static RBX::NumberSequence::Key convert(const G3D::Vector4& a)
{
    return RBX::NumberSequence::Key(a.x, a.y, a.z);
}

static G3D::Vector4 convert(const RBX::NumberSequence::Key& k, int j )
{
    return G3D::Vector4( k.time, k.value, k.envelope, 0.1f + j );
}

static void convert(SplineDef* dest, const RBX::NumberSequence* src, float ymax)
{
    const std::vector<RBX::NumberSequence::Key>& keys = src->getPoints();
    dest->ymax = ymax;
    dest->pt.resize(keys.size());
    for( int j=0, e=keys.size(); j<e; ++j )
    {
        dest->pt[j] = convert(keys[j], j);
        dest->pt[j].x = clamp(dest->pt[j].x, 0, 1);
        dest->pt[j].y = clamp(dest->pt[j].y, 0, ymax);
        dest->pt[j].z = clamp(dest->pt[j].z, 0, dest->pt[j].y );
        dest->pt[j].z = clamp(dest->pt[j].z, 0, ymax - dest->pt[j].y );
    }
}

static void convert(RBX::NumberSequence* dest, const SplineDef* src)
{
    std::vector<RBX::NumberSequence::Key> keys;
    keys.reserve( src->pt.size() );
    for( int j=0, e=src->pt.size(); j<e; ++j )
    {
        RBX::NumberSequence::Key key = convert(src->pt[j]);
        if (key.time >= 0.0f && key.time <= 1.0f)
            keys.push_back(key);
    }
    if (!RBX::NumberSequence::validate(keys, false))
        RBXASSERT(RBX::NumberSequence::validate(keys, false)); // if this happens, then the editor is supplying a borked curve and this conversion function is not smart enough to filter it
    *dest = keys;
}

SplineEditorAdapter::SplineEditorAdapter( const RBX::Reflection::PropertyDescriptor* pPropertyDescriptor, boost::shared_ptr<RBX::DataModel> dataModel, RBX::Instance* inst )
    : m_pPropertyDescriptor(pPropertyDescriptor)
    , m_dataModel(dataModel)
    , m_pInst(inst)
    , m_inhibitPropChanged(false)
{
    const char* settingsName = "generic";
    float ymax = 1; // default


    // this sucks, but until we get a better way to communicate the max. range of the values in the sequence, the code below shall remain:
    if (pPropertyDescriptor == &RBX::CustomParticleEmitter::prop_transp)
    {
        ymax = 1;
        settingsName = "Transparency";
    }
    else if (pPropertyDescriptor == &RBX::CustomParticleEmitter::prop_size)
    {
        ymax = 10;
        settingsName = "Size";
    }

    RBX::Reflection::Variant variant;
    m_pPropertyDescriptor->getVariant( m_pInst, variant );
    RBX::NumberSequence ns = variant.get<RBX::NumberSequence>();
    convert( &m_spline, &ns, ymax );

    m_pSplineEditor = new SplineEditor(&UpdateUIManager::Instance().getMainWindow(), &m_spline, settingsName);

    if (1) // set title
    {
        QString title = QString::fromStdString(inst->getFullName() + "." + m_pPropertyDescriptor->name.str);
        m_pSplineEditor->setWindowTitle(title);
    }

    UpdateUIManager::Instance().getMainWindow().addDockWidget(Qt::BottomDockWidgetArea, m_pSplineEditor );

    connect( m_pSplineEditor, SIGNAL(splineEdited(bool)), this, SLOT(onEdited(bool)));
    connect( m_pSplineEditor, SIGNAL(finished()), this, SLOT(onClosed()));

    m_pSplineEditor->setFocus();
  
    m_propChangedConn = m_pInst->propertyChangedSignal.connect( boost::bind(&SplineEditorAdapter::onPropChanged, this, _1) );
    m_ancestryChangedConn = m_pInst->ancestryChangedSignal.connect( boost::bind(&SplineEditorAdapter::onAncestryChanged, this, _1, _2) );
}

SplineEditorAdapter::~SplineEditorAdapter()
{
    m_pSplineEditor->deleteLater();
    disconnect( this, SLOT(onEdited(bool)) );
    disconnect( this, SLOT(onClosed()) );
};

void SplineEditorAdapter::show()
{
    m_pSplineEditor->show();
}

void SplineEditorAdapter::onEdited(bool undo)
{
    RBX::NumberSequence ns;
    convert( &ns, &m_spline );

    m_inhibitPropChanged = true;

    RBX::DataModel::LegacyLock lock(m_dataModel.get(), RBX::DataModelJob::Write);
    m_pPropertyDescriptor->setVariant(m_pInst, ns);

    if( undo )
    {
        RBX::ChangeHistoryService::requestWaypoint(m_pPropertyDescriptor->name.c_str(), m_dataModel.get());
    }

    m_inhibitPropChanged = false;
}

void SplineEditorAdapter::onPropChanged(const RBX::Reflection::PropertyDescriptor* pd)
{
    if( !m_inhibitPropChanged && pd == m_pPropertyDescriptor )
    {
        RBX::Reflection::Variant variant;
        m_pPropertyDescriptor->getVariant( m_pInst, variant );
        RBX::NumberSequence ns = variant.get<RBX::NumberSequence>();

        SplineDef* sd = new SplineDef();
        convert( sd, &ns, m_spline.ymax );

        QMetaObject::invokeMethod(m_pSplineEditor, "refreshData", Qt::QueuedConnection, Q_ARG(void*, sd) ); // this sucks, Qt can't into arbitrary types
    }
}

void SplineEditorAdapter::onAncestryChanged(shared_ptr<RBX::Instance>, shared_ptr<RBX::Instance>)
{
    RBX::DataModel* dm = RBX::DataModel::get(m_pInst);
    if (!dm) // we're effectively dead
    {
        m_pSplineEditor->close();
        return;
    }
}

void SplineEditorAdapter::onClosed()
{
    onEdited(true);
    delete this;
}





























