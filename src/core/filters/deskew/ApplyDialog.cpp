// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ApplyDialog.h"

#include "PageSelectionAccessor.h"

namespace deskew {
ApplyDialog::ApplyDialog(QWidget* parent, const PageId& curPage, const PageSelectionAccessor& pageSelectionAccessor)
    : QDialog(parent),
      m_pages(pageSelectionAccessor.allPages()),
      m_curPage(curPage),
      m_selectedPages(pageSelectionAccessor.selectedPages()),
      m_scopeGroup(new QButtonGroup(this)) {
  setupUi(this);
  m_scopeGroup->addButton(thisPageRB);
  m_scopeGroup->addButton(allPagesRB);
  m_scopeGroup->addButton(thisPageAndFollowersRB);
  m_scopeGroup->addButton(everyOtherRB);
  m_scopeGroup->addButton(thisEveryOtherRB);
  m_scopeGroup->addButton(selectedPagesRB);
  m_scopeGroup->addButton(everyOtherSelectedRB);
  if (m_selectedPages.size() <= 1) {
    selectedPagesRB->setEnabled(false);
    selectedPagesHint->setEnabled(false);
    everyOtherSelectedRB->setEnabled(false);
    everyOtherSelectedHint->setEnabled(false);
  }

  connect(buttonBox, SIGNAL(accepted()), this, SLOT(onSubmit()));
}

ApplyDialog::~ApplyDialog() = default;

void ApplyDialog::onSubmit() {
  const bool applyDeskew = applyDeskewCheckBox->isChecked();
  const bool applyOblique = applyObliqueCheckBox->isChecked();

  std::set<PageId> pages;
  // thisPageRB is intentionally not handled.
  if (allPagesRB->isChecked()) {
    m_pages.selectAll().swap(pages);
    emit appliedToAllPages(pages, applyDeskew, applyOblique);
  } else if (thisPageAndFollowersRB->isChecked()) {
    m_pages.selectPagePlusFollowers(m_curPage).swap(pages);
    emit appliedTo(pages, applyDeskew, applyOblique);
  } else if (selectedPagesRB->isChecked()) {
    emit appliedTo(m_selectedPages, applyDeskew, applyOblique);
  } else if (everyOtherRB->isChecked()) {
    m_pages.selectEveryOther(m_curPage).swap(pages);
    emit appliedTo(pages, applyDeskew, applyOblique);
  } else if (thisEveryOtherRB->isChecked()) {
    m_pages.selectThisPageAndFollowingEveryOther(m_curPage).swap(pages);
    emit appliedTo(pages, applyDeskew, applyOblique);
  } else if (everyOtherSelectedRB->isChecked()) {
    m_pages.selectEveryOtherInSubsetFromPage(m_curPage, m_selectedPages).swap(pages);
    emit appliedTo(pages, applyDeskew, applyOblique);
  }
  accept();
}  // ApplyDialog::onSubmit
}  // namespace deskew