/* This file is part of Dilay
 * Copyright © 2015-2018 Alexander Bau
 * Use and redistribute under the terms of the GNU General Public License
 */
#include <QKeySequence>
#include <QObject>
#include "../util.hpp"
#include "view/tool-tip.hpp"

struct ViewToolTip::Impl
{
  typedef std::tuple<ViewInputEvent, ViewInputModifier, QString> Tip;
  std::vector<Tip>                                               tips;

  void render (const std::function<void(const QString&, const QString&)>& f) const
  {
    for (auto& t : this->tips)
    {
      f (ViewInput::toQString (std::get<0> (t), std::get<1> (t)), std::get<2> (t));
    }
  }

  void add (ViewInputEvent event, ViewInputModifier modifier, const QString& tip)
  {
    this->tips.push_back (std::make_tuple (event, modifier, tip));
  }

  void add (ViewInputEvent event, const QString& tip)
  {
    this->add (event, ViewInputModifier::None, tip);
  }

  void reset () { this->tips.clear (); }

  bool isEmpty () const { return this->tips.empty (); }
};

DELEGATE_BIG6 (ViewToolTip)
DELEGATE1_CONST (void, ViewToolTip, render,
                 const std::function<void(const QString&, const QString&)>&)
DELEGATE3 (void, ViewToolTip, add, ViewInputEvent, ViewInputModifier, const QString&)
DELEGATE2 (void, ViewToolTip, add, ViewInputEvent, const QString&)
DELEGATE (void, ViewToolTip, reset)
DELEGATE_CONST (bool, ViewToolTip, isEmpty)
