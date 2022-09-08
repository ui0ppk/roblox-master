
#pragma once

//this will make sure that we can use qt with boost (else moc compiler errors)
//without it, qt moc interprets 'rbx::signals::connection foo' as 'rbx::    signals:   :connection foo' and chokes to death
#ifndef QT_NO_KEYWORDS
#   define QT_NO_KEYWORDS
#endif

#include <QDialog>
#include <QTextDocument>
#include <QRegExp>
#include <QComboBox>
#include <QDockWidget>
#include "rbx/signal.h"
#include "G3D/Vector4.h"

class QLineEdit;
class QPushButton;
class QGroupBox;

class  SplineEditorCanvas;

struct SplineDef
{
    std::vector< G3D::Vector4 > pt; // .xy = point coordinates, .z = envelope, .w = point id
    float ymax; // max. allowed y-value for points, scales the plot based on this
};


class SplineEditor: public QDockWidget
{
    Q_OBJECT

public:
    typedef QDockWidget Base;

    SplineEditor(QWidget *parent, SplineDef* def, const char* settingsName);
    ~SplineEditor();

public Q_SLOTS:
    void refreshData(void* sd); // pulls the new data from the spline, discards dragging, etc.


public: // private actually
    void onCanvasMouse( QMouseEvent* e );
    void onCanvasLeave( QEvent* e );

Q_SIGNALS:
    void splineEdited(bool undoPoint); // called when spline has been modified by the user. 
    void finished();

private:
    SplineDef* m_spline;
    SplineDef* m_splineBackup; // for reset
    SplineEditorCanvas* m_pCanvasWidget;
    QPushButton* m_pResetButton;
    QPushButton* m_pCloseButton;
    QLineEdit* m_pXEdit;
    QLineEdit* m_pYEdit;
    QLineEdit* m_pEnvEdit;
    QPushButton* m_pKillPtButton;
    QFrame*  m_pRootFrame;
    QString m_settingsName;

    enum Mode // mouse interaction
    {
        Bad, // not used
        Free,   // just hovering around
        Drag,   // dragging an existing handle
        DragNew, // drag-creating a new handle 
        DragEnv, // dragging an envelope
        DragEnvHandle, // dragging an envelope handle
    };

    enum Selection
    {
        Sel_None,
        Sel_Point,
        Sel_EnvUpper,
        Sel_EnvLower,
    };

    int m_sel;
    int m_selEnvHandle;
    Mode m_mode;
    int  m_lastid;
    G3D::Vector4 m_savePt;
    int  m_clickX, m_clickY;
    bool m_keepHighlight;
    float m_Xconstraint;

    void syncPoint(int sel, bool direction); // direction: true: write point data to widgets, false: read data from widgets
    int  pointById(int id); // returns index

    G3D::Vector4* newPoint(float x, float y, float e);
    void  killPoint(int id);
    Selection updateSelection(int x, int y);
    void  sort();
    void  constrainValue(G3D::Vector4* pt); // constrains the point value to be in spline's range
    void  constrainEnvelope(G3D::Vector4* pt); // constrains the envelope to be in spline's range
    float autoEnvelope( float x, float y ); // helper, computes automatic envelope based on neighboring points
    int   leftmostPoint(); // returns id of the leftmost point
    int   rightmostPoint(); // returns id of the rightmost point
    void  updateGhosting(int cx, int cy, bool show); //

    void  select( int id ); // selects a point
    int   getsel();

    void closeEvent(QCloseEvent *event);

    virtual void keyPressEvent(QKeyEvent *);
    virtual bool event(QEvent* ev);
        
private Q_SLOTS:
    virtual void onPointDataChanged();
    virtual void onKillPtClicked();
    virtual void onClose();
    virtual void onReset();
    
};



class SplineEditorCanvas : public QWidget
{
    Q_OBJECT;
public:
    SplineEditorCanvas(QWidget* parent, SplineEditor* owner, SplineDef* spline);
    ~SplineEditorCanvas();

    void paintEvent(QPaintEvent * e);
    void select(int ptid) { m_selPt = ptid; } // marks the given point as selected
    void setHighlight(int what) { m_highlight_ = what; } // 0 = none, 1 - point, 2 - upper size handle, 3 - lower size handle
    int  getPointAt(int x, int y); // returns the point id at the given coordinates, (-1) if not found
    std::pair<int,int> getEnvHandleAt(int x, int y); // returns the point id of the size handle and the size handle type (0 = upper, 1 = lower) at the given coordinates, (-1) if not found


    void ghost( float x, float y, float e) { m_ghostX = x; m_ghostY = y; m_ghostE = e; } // updates the ghost, x=-1 to disable

    void cvt( float* fx, float* fy, int x, int y ); // converts canvas to normalized coordinates
    void cvt( int* ix, int* iy, float x, float y ); // converts normalized to canvas coordinates

private:
    SplineDef* m_spline;
    SplineEditor* m_pOwner;
    int        m_selPt;
    int        m_highlight_;
    float      m_ghostX, m_ghostY, m_ghostE;

    virtual void mousePressEvent(QMouseEvent * e);
    virtual void mouseReleaseEvent(QMouseEvent * e );
    virtual void mouseMoveEvent(QMouseEvent * e);
    virtual void leaveEvent( QEvent* e );
};

namespace RBX { class Instance; class NumberSequence; }
namespace RBX { namespace Reflection { class PropertyDescriptor; } }
namespace RBX { class DataModel; }

// used to edit NumberSequences
class SplineEditorAdapter : public QObject
{
    Q_OBJECT;
public:
    SplineEditorAdapter( const RBX::Reflection::PropertyDescriptor* pPropertyDescriptor, boost::shared_ptr<RBX::DataModel> dataModel, RBX::Instance* inst );
    ~SplineEditorAdapter();

    void show();


private:
    const RBX::Reflection::PropertyDescriptor* m_pPropertyDescriptor;
    shared_ptr<RBX::DataModel> m_dataModel;
    RBX::Instance* m_pInst;
    SplineEditor* m_pSplineEditor;
    SplineDef m_spline;
    rbx::signals::scoped_connection m_propChangedConn, m_ancestryChangedConn;
    bool m_inhibitPropChanged;

private Q_SLOTS:
    void onEdited(bool undo); // called on interactive changes, undo = true means 'submit to undo buffer'
    void onClosed(); // called when the dialog is closed
    void onPropChanged(const RBX::Reflection::PropertyDescriptor* pd); // called when the property changes externally to refresh the stuff
    void onAncestryChanged(shared_ptr<RBX::Instance>, shared_ptr<RBX::Instance>);
};
