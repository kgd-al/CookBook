#ifndef ANDROIDSPECIFICS_HPP
#define ANDROIDSPECIFICS_HPP

#include <QDebug>

#ifdef Q_OS_ANDROID
#include <QGestureEvent>
#include <QGestureRecognizer>
#include <QtMath>

namespace android {

class ExtendedSwipeGesture : public QGesture {
public:
  ExtendedSwipeGesture (QObject *object) : QGesture(object) {}

  void reset (void) {
    setStart(QPointF());
  }

  void setStart (const QPointF &p) {
    setHotSpot(p);
  }

  void setEnd (const QPointF &p) {
    auto diff = p - hotSpot();
    _dx = diff.x();
    _dy = diff.y();
  }

  float dx (void) const { return _dx; }
  float dy (void) const { return _dy; }

  float xMagnitude (void) const { return fabs(_dx); }
  float yMagnitude (void) const { return fabs(_dy); }
  float magnitude (void) const {  return qMax(xMagnitude(), yMagnitude()); }

private:
  float _dx, _dy;
};

class SingleFingerSwipeRecognizer : public QGestureRecognizer {
public:
  static Qt::GestureType type (void) {
    static auto type =
        QGestureRecognizer::registerRecognizer(new SingleFingerSwipeRecognizer);
    return type;
  }

private:
  using G = ExtendedSwipeGesture;
  QMap<QObject*, ResultFlag> activeMonitors;

  QGesture* create (QObject *target) {
    return new ExtendedSwipeGesture (target);
  }

  void reset (QGesture *state) {
    static_cast<G*>(state)->reset();
  }

  Result recognize(QGesture *state, QObject *watched, QEvent *event) {
    if (event->type() == QEvent::TouchBegin
        && !activeMonitors.contains(watched)) {
      static_cast<G*>(state)->setHotSpot(
        static_cast<QTouchEvent*>(event)->touchPoints().front().pos());
//      qDebug() << __PRETTY_FUNCTION__ << "(" << state << watched << event
//               << "):\n\tMay start at "
//               << static_cast<QTouchEvent*>(event)->touchPoints().front().pos();
      activeMonitors.insert(watched, MayBeGesture);
      return MayBeGesture;

    } else if (event->type() == QEvent::TouchEnd
               && activeMonitors.contains(watched)) {
      auto sg = static_cast<G*>(state);
      sg->setEnd(static_cast<QTouchEvent*>(event)->touchPoints().front().pos());
      ResultFlag flag = CancelGesture;
      if (sg->magnitude() > 100) {
//        qDebug() << __PRETTY_FUNCTION__ << "(" << state << watched << event
//                 << "):\n\tSwipe end "
//                 << sg->hotSpot() << " + (" << sg->dx() << "," << sg->dy()
//                 << ")";
        flag = FinishGesture;
      }

      activeMonitors.remove(watched);
      return flag;
    }

    return Ignore;
  }
};

} // end of namespace android

#endif

#endif // ANDROIDSPECIFICS_HPP
