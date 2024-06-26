// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "FixDpiDialog.h"

#include <QSortFilterProxyModel>
#include <boost/foreach.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>

#include "ColorSchemeManager.h"

// To be able to use it in QVariant
Q_DECLARE_METATYPE(ImageMetadata)

static const int NEED_FIXING_TAB = 0;
static const int ALL_PAGES_TAB = 1;

// Requests a group of ImageMetadata objects folded into one.
static const int AGGREGATE_METADATA_ROLE = Qt::UserRole;
// Same as the one above, but only objects with .isDpiOK() == false
// will be considered.
static const int AGGREGATE_NOT_OK_METADATA_ROLE = Qt::UserRole + 1;


/**
 * This class computes an aggregate ImageMetadata object from a group of other
 * ImageMetadata objects.  If all ImageMetadata objects in a group are equal,
 * that will make it an aggregate metadata.  Otherwise, a null (default
 * constructed) ImageMetadata() object will be considered
 * the DPIs within the group are not consistent,
 * the aggregate Image metadata object will have zeros both for size and for
 * DPI values.  If the DPIs are consistent but sizes are not, the aggregate
 * ImageMetadata will have the consistent DPI and zero size.
 */
class FixDpiDialog::DpiCounts {
 public:
  void add(const ImageMetadata& metadata);

  void remove(const ImageMetadata& metadata);

  /**
   * Checks if all ImageMetadata objects return true for ImageMetadata::isDpiOK().
   */
  bool allDpisOK() const;

  /**
   * If all ImageMetadata objects are equal, one of them will be returned.
   * Otherwise, a default-constructed ImageMetadata() object will be returned.
   */
  ImageMetadata aggregate(Scope scope) const;

 private:
  struct MetadataComparator {
    bool operator()(const ImageMetadata& lhs, const ImageMetadata& rhs) const;
  };

  using Map = std::map<ImageMetadata, int, MetadataComparator>;

  Map m_counts;
};


/**
 * This comparator puts objects that are not OK to the front.
 */
bool FixDpiDialog::DpiCounts::MetadataComparator::operator()(const ImageMetadata& lhs, const ImageMetadata& rhs) const {
  const bool lhsOk = lhs.isDpiOK();
  const bool rhsOk = rhs.isDpiOK();
  if (lhsOk != rhsOk) {
    return rhsOk;
  }

  if (lhs.size().width() < rhs.size().width()) {
    return true;
  } else if (lhs.size().width() > rhs.size().width()) {
    return false;
  } else if (lhs.size().height() < rhs.size().height()) {
    return true;
  } else if (lhs.size().height() > rhs.size().height()) {
    return false;
  } else if (lhs.dpi().horizontal() < rhs.dpi().horizontal()) {
    return true;
  } else if (lhs.dpi().horizontal() > rhs.dpi().horizontal()) {
    return false;
  } else {
    return lhs.dpi().vertical() < rhs.dpi().vertical();
  }
}

class FixDpiDialog::SizeGroup {
 public:
  struct Item {
    int fileIdx;
    int imageIdx;

    Item(int fileIdx, int imageIdx) : fileIdx(fileIdx), imageIdx(imageIdx) {}
  };

  explicit SizeGroup(const QSize& size) : m_size(size) {}

  void append(const Item& item, const ImageMetadata& metadata);

  const QSize& size() const { return m_size; }

  const std::vector<Item>& items() const { return m_items; }

  DpiCounts& dpiCounts() { return m_dpiCounts; }

  const DpiCounts& dpiCounts() const { return m_dpiCounts; }

 private:
  QSize m_size;
  std::vector<Item> m_items;
  DpiCounts m_dpiCounts;
};


class FixDpiDialog::TreeModel : private QAbstractItemModel {
 public:
  explicit TreeModel(const std::vector<ImageFileInfo>& files);

  const std::vector<ImageFileInfo>& files() const { return m_files; }

  QAbstractItemModel* model() { return this; }

  bool allDpisOK() const { return m_dpiCounts.allDpisOK(); }

  bool isVisibleForFilter(const QModelIndex& parent, int row) const;

  void applyDpiToSelection(Scope scope, const Dpi& dpi, const QItemSelection& selection);

 private:
  struct Tag {};

  int columnCount(const QModelIndex& parent) const override;

  int rowCount(const QModelIndex& parent) const override;

  QModelIndex index(int row, int column, const QModelIndex& parent) const override;

  QModelIndex parent(const QModelIndex& index) const override;

  QVariant data(const QModelIndex& index, int role) const override;

  void applyDpiToAllGroups(Scope scope, const Dpi& dpi);

  void applyDpiToGroup(Scope scope, const Dpi& dpi, SizeGroup& group, DpiCounts& totalDpiCounts);

  void applyDpiToItem(Scope scope,
                      const ImageMetadata& newMetadata,
                      SizeGroup::Item item,
                      DpiCounts& totalDpiCounts,
                      DpiCounts& groupDpiCounts);

  void emitAllPagesChanged(const QModelIndex& idx);

  void emitSizeGroupChanged(const QModelIndex& idx);

  void emitItemChanged(const QModelIndex& idx);

  SizeGroup& sizeGroupFor(QSize size);

  static QString sizeToString(QSize size);

  static Tag m_allPagesNodeId;
  static Tag m_sizeGroupNodeId;

  std::vector<ImageFileInfo> m_files;
  std::vector<SizeGroup> m_sizes;
  DpiCounts m_dpiCounts;
};


class FixDpiDialog::FilterModel : private QSortFilterProxyModel {
 public:
  explicit FilterModel(TreeModel& delegate);

  QAbstractProxyModel* model() { return this; }

 private:
  bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

  QVariant data(const QModelIndex& index, int role) const override;

  TreeModel& m_delegate;
};


FixDpiDialog::FixDpiDialog(const std::vector<ImageFileInfo>& files, QWidget* parent)
    : QDialog(parent),
      m_pages(std::make_unique<TreeModel>(files)),
      m_undefinedDpiPages(std::make_unique<FilterModel>(*m_pages)) {
  setupUi(this);

  m_normalPalette = xDpi->palette();
  m_errorPalette = m_normalPalette;
  const QColor errorTextColor = ColorSchemeManager::instance().getColorParam("FixDpiDialogErrorText", QColor(Qt::red));
  m_errorPalette.setColor(QPalette::Text, errorTextColor);

  dpiCombo->addItem("300 x 300", QSize(300, 300));
  dpiCombo->addItem("400 x 400", QSize(400, 400));
  dpiCombo->addItem("600 x 600", QSize(600, 600));
  dpiCombo->addItem("1200 x 1200", QSize(1200, 1200));

  tabWidget->setTabText(NEED_FIXING_TAB, tr("Need Fixing"));
  tabWidget->setTabText(ALL_PAGES_TAB, tr("All Pages"));
  undefinedDpiView->setModel(m_undefinedDpiPages->model()), undefinedDpiView->header()->hide();
  allPagesView->setModel(m_pages->model());
  allPagesView->header()->hide();

  xDpi->setMaxLength(4);
  yDpi->setMaxLength(4);
  xDpi->setValidator(new QIntValidator(xDpi));
  yDpi->setValidator(new QIntValidator(yDpi));

  connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));

  connect(undefinedDpiView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
          this, SLOT(selectionChanged(const QItemSelection&)));
  connect(allPagesView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this,
          SLOT(selectionChanged(const QItemSelection&)));

  connect(dpiCombo, SIGNAL(activated(int)), this, SLOT(dpiComboChangedByUser(int)));

  connect(xDpi, SIGNAL(textEdited(const QString&)), this, SLOT(dpiValueChanged()));
  connect(yDpi, SIGNAL(textEdited(const QString&)), this, SLOT(dpiValueChanged()));

  connect(applyBtn, SIGNAL(clicked()), this, SLOT(applyClicked()));

  enableDisableOkButton();
}

FixDpiDialog::~FixDpiDialog() = default;

const std::vector<ImageFileInfo>& FixDpiDialog::files() const {
  return m_pages->files();
}

void FixDpiDialog::tabChanged(const int tab) {
  QTreeView* views[2];
  views[NEED_FIXING_TAB] = undefinedDpiView;
  views[ALL_PAGES_TAB] = allPagesView;
  updateDpiFromSelection(views[tab]->selectionModel()->selection());
}

void FixDpiDialog::selectionChanged(const QItemSelection& selection) {
  updateDpiFromSelection(selection);
}

void FixDpiDialog::dpiComboChangedByUser(const int index) {
  const QVariant data(dpiCombo->itemData(index));
  if (data.isValid()) {
    const QSize dpi(data.toSize());
    xDpi->setText(QString::number(dpi.width()));
    yDpi->setText(QString::number(dpi.height()));
    dpiValueChanged();
  }
}

void FixDpiDialog::dpiValueChanged() {
  updateDpiCombo();

  const Dpi dpi(xDpi->text().toInt(), yDpi->text().toInt());
  const ImageMetadata metadata(m_selectedItemPixelSize, dpi);

  decorateDpiInputField(xDpi, metadata.horizontalDpiStatus());
  decorateDpiInputField(yDpi, metadata.verticalDpiStatus());

  if ((m_xDpiInitialValue == xDpi->text()) && (m_yDpiInitialValue == yDpi->text())) {
    applyBtn->setEnabled(false);
    return;
  }


  if (metadata.isDpiOK()) {
    applyBtn->setEnabled(true);
    return;
  }

  applyBtn->setEnabled(false);
}

void FixDpiDialog::applyClicked() {
  const Dpi dpi(xDpi->text().toInt(), yDpi->text().toInt());
  QItemSelectionModel* selectionModel = nullptr;

  if (tabWidget->currentIndex() == ALL_PAGES_TAB) {
    selectionModel = allPagesView->selectionModel();
    const QItemSelection selection(selectionModel->selection());
    m_pages->applyDpiToSelection(ALL, dpi, selection);
  } else {
    selectionModel = undefinedDpiView->selectionModel();
    const QItemSelection selection(m_undefinedDpiPages->model()->mapSelectionToSource(selectionModel->selection()));
    m_pages->applyDpiToSelection(NOT_OK, dpi, selection);
  }

  updateDpiFromSelection(selectionModel->selection());
  enableDisableOkButton();
}

void FixDpiDialog::enableDisableOkButton() {
  const bool enable = m_pages->allDpisOK();
  buttonBox->button(QDialogButtonBox::Ok)->setEnabled(enable);
}

/**
 * This function work with both TreeModel and FilterModel selections.
 * It is assumed that only a single item is selected.
 */
void FixDpiDialog::updateDpiFromSelection(const QItemSelection& selection) {
  if (selection.isEmpty()) {
    resetDpiForm();
    dpiCombo->setEnabled(false);
    xDpi->setEnabled(false);
    yDpi->setEnabled(false);
    // applyBtn is managed elsewhere.
    return;
  }

  dpiCombo->setEnabled(true);
  xDpi->setEnabled(true);
  yDpi->setEnabled(true);

  // FilterModel may replace AGGREGATE_METADATA_ROLE with AGGREGATE_NOT_OK_METADATA_ROLE.
  const QVariant data(selection.front().topLeft().data(AGGREGATE_METADATA_ROLE));
  if (data.isValid()) {
    setDpiForm(data.value<ImageMetadata>());
  } else {
    resetDpiForm();
  }
}

void FixDpiDialog::resetDpiForm() {
  dpiCombo->setCurrentIndex(0);
  m_xDpiInitialValue.clear();
  m_yDpiInitialValue.clear();
  xDpi->setText(m_xDpiInitialValue);
  yDpi->setText(m_yDpiInitialValue);
  dpiValueChanged();
}

void FixDpiDialog::setDpiForm(const ImageMetadata& metadata) {
  const Dpi dpi(metadata.dpi());

  if (dpi.isNull()) {
    resetDpiForm();
    return;
  }

  m_xDpiInitialValue = QString::number(dpi.horizontal());
  m_yDpiInitialValue = QString::number(dpi.vertical());
  m_selectedItemPixelSize = metadata.size();
  xDpi->setText(m_xDpiInitialValue);
  yDpi->setText(m_yDpiInitialValue);
  dpiValueChanged();
}

void FixDpiDialog::updateDpiCombo() {
  bool xOk = true, y_ok = true;
  const QSize dpi(xDpi->text().toInt(&xOk), yDpi->text().toInt(&y_ok));

  if (xOk && y_ok) {
    const int count = dpiCombo->count();
    for (int i = 0; i < count; ++i) {
      const QVariant data(dpiCombo->itemData(i));
      if (data.isValid()) {
        if (dpi == data.toSize()) {
          dpiCombo->setCurrentIndex(i);
          return;
        }
      }
    }
  }

  dpiCombo->setCurrentIndex(0);
}

void FixDpiDialog::decorateDpiInputField(QLineEdit* field, ImageMetadata::DpiStatus dpiStatus) const {
  if (dpiStatus == ImageMetadata::DPI_OK) {
    field->setPalette(m_normalPalette);
  } else {
    field->setPalette(m_errorPalette);
  }

  switch (dpiStatus) {
    case ImageMetadata::DPI_OK:
    case ImageMetadata::DPI_UNDEFINED:
      field->setToolTip(QString());
      break;
    case ImageMetadata::DPI_TOO_LARGE:
      field->setToolTip(tr("DPI is too large and most likely wrong."));
      break;
    case ImageMetadata::DPI_TOO_SMALL:
      field->setToolTip(
          tr("DPI is too small. Even if it's correct, you are not going to get acceptable results with it."));
      break;
    case ImageMetadata::DPI_TOO_SMALL_FOR_THIS_PIXEL_SIZE:
      field->setToolTip(
          tr("DPI is too small for this pixel size. Such combination would probably lead to out of "
             "memory errors."));
      break;
  }
}

/*====================== FixDpiDialog::DpiCounts ======================*/

void FixDpiDialog::DpiCounts::add(const ImageMetadata& metadata) {
  ++m_counts[metadata];
}

void FixDpiDialog::DpiCounts::remove(const ImageMetadata& metadata) {
  if (--m_counts[metadata] == 0) {
    m_counts.erase(metadata);
  }
}

bool FixDpiDialog::DpiCounts::allDpisOK() const {
  // We put wrong DPIs to the front, so if the first one is OK,
  // the others are OK as well.
  const auto it(m_counts.begin());
  return it == m_counts.end() || it->first.isDpiOK();
}

ImageMetadata FixDpiDialog::DpiCounts::aggregate(const Scope scope) const {
  const auto it(m_counts.begin());

  if (it == m_counts.end()) {
    return ImageMetadata();
  }

  if ((scope == NOT_OK) && it->first.isDpiOK()) {
    // If this one is OK, the following ones are OK as well.
    return ImageMetadata();
  }

  Map::const_iterator next(it);
  ++next;

  if (next == m_counts.end()) {
    return it->first;
  }

  if ((scope == NOT_OK) && next->first.isDpiOK()) {
    // If this one is OK, the following ones are OK as well.
    return it->first;
  }
  return ImageMetadata();
}

/*====================== FixDpiDialog::SizeGroup ======================*/

void FixDpiDialog::SizeGroup::append(const Item& item, const ImageMetadata& metadata) {
  m_items.push_back(item);
  m_dpiCounts.add(metadata);
}

/*====================== FixDpiDialog::TreeModel ======================*/

FixDpiDialog::TreeModel::Tag FixDpiDialog::TreeModel::m_allPagesNodeId;
FixDpiDialog::TreeModel::Tag FixDpiDialog::TreeModel::m_sizeGroupNodeId;

FixDpiDialog::TreeModel::TreeModel(const std::vector<ImageFileInfo>& files) : m_files(files) {
  const auto numFiles = static_cast<int>(m_files.size());
  for (int i = 0; i < numFiles; ++i) {
    const ImageFileInfo& file = m_files[i];
    const auto numImages = static_cast<int>(file.imageInfo().size());
    for (int j = 0; j < numImages; ++j) {
      const ImageMetadata& metadata = file.imageInfo()[j];
      SizeGroup& group = sizeGroupFor(metadata.size());
      group.append(SizeGroup::Item(i, j), metadata);
      m_dpiCounts.add(metadata);
    }
  }
}

bool FixDpiDialog::TreeModel::isVisibleForFilter(const QModelIndex& parent, int row) const {
  const void* const ptr = parent.internalPointer();

  if (!parent.isValid()) {
    // 'All Pages'.
    return !m_dpiCounts.allDpisOK();
  } else if (ptr == &m_allPagesNodeId) {
    // A size group.
    return !m_sizes[row].dpiCounts().allDpisOK();
  } else if (ptr == &m_sizeGroupNodeId) {
    // An image.
    const SizeGroup& group = m_sizes[parent.row()];
    const SizeGroup::Item& item = group.items()[row];
    const ImageFileInfo& file = m_files[item.fileIdx];
    return !file.imageInfo()[item.imageIdx].isDpiOK();
  } else {
    // Should not happen.
    return false;
  }
}

void FixDpiDialog::TreeModel::applyDpiToSelection(const Scope scope, const Dpi& dpi, const QItemSelection& selection) {
  if (selection.isEmpty()) {
    return;
  }

  const QModelIndex parent(selection.front().parent());
  const int row = selection.front().top();
  const void* const ptr = parent.internalPointer();
  const QModelIndex idx(index(row, 0, parent));

  if (!parent.isValid()) {
    // Apply to all pages.
    applyDpiToAllGroups(scope, dpi);
    emitAllPagesChanged(idx);
  } else if (ptr == &m_allPagesNodeId) {
    // Apply to a size group.
    SizeGroup& group = m_sizes[row];
    applyDpiToGroup(scope, dpi, group, m_dpiCounts);
    emitSizeGroupChanged(index(row, 0, parent));
  } else if (ptr == &m_sizeGroupNodeId) {
    // Images within a size group.
    SizeGroup& group = m_sizes[parent.row()];
    const SizeGroup::Item& item = group.items()[row];
    const ImageMetadata metadata(group.size(), dpi);
    applyDpiToItem(scope, metadata, item, m_dpiCounts, group.dpiCounts());
    emitItemChanged(idx);
  }
}

int FixDpiDialog::TreeModel::columnCount(const QModelIndex& parent) const {
  return 1;
}

int FixDpiDialog::TreeModel::rowCount(const QModelIndex& parent) const {
  const void* const ptr = parent.internalPointer();

  if (!parent.isValid()) {
    // The single 'All Pages' item.
    return 1;
  } else if (ptr == &m_allPagesNodeId) {
    // Size groups.
    return static_cast<int>(m_sizes.size());
  } else if (ptr == &m_sizeGroupNodeId) {
    // Images within a size group.
    return static_cast<int>(m_sizes[parent.row()].items().size());
  } else {
    // Children of an image.
    return 0;
  }
}

QModelIndex FixDpiDialog::TreeModel::index(const int row, const int column, const QModelIndex& parent) const {
  const void* const ptr = parent.internalPointer();

  if (!parent.isValid()) {
    // The 'All Pages' item.
    return createIndex(row, column, &m_allPagesNodeId);
  } else if (ptr == &m_allPagesNodeId) {
    // A size group.
    return createIndex(row, column, &m_sizeGroupNodeId);
  } else if (ptr == &m_sizeGroupNodeId) {
    // An image within some size group.
    return createIndex(row, column, (void*) &m_sizes[parent.row()]);
  }
  return QModelIndex();
}

QModelIndex FixDpiDialog::TreeModel::parent(const QModelIndex& index) const {
  const void* const ptr = index.internalPointer();

  if (!index.isValid()) {
    // Should not happen.
    return QModelIndex();
  } else if (ptr == &m_allPagesNodeId) {
    // 'All Pages' -> tree root.
    return QModelIndex();
  } else if (ptr == &m_sizeGroupNodeId) {
    // Size group -> 'All Pages'.
    return createIndex(0, index.column(), &m_allPagesNodeId);
  } else {
    // Image -> size group.
    const auto* group = static_cast<const SizeGroup*>(ptr);
    return createIndex(static_cast<int>(group - &m_sizes[0]), index.column(), &m_sizeGroupNodeId);
  }
}

QVariant FixDpiDialog::TreeModel::data(const QModelIndex& index, const int role) const {
  const void* const ptr = index.internalPointer();

  if (!index.isValid()) {
    // Should not happen.
    return QVariant();
  } else if (ptr == &m_allPagesNodeId) {
    // 'All Pages'.
    if (role == Qt::DisplayRole) {
      return FixDpiDialog::tr("All Pages");
    } else if (role == AGGREGATE_METADATA_ROLE) {
      return QVariant::fromValue(m_dpiCounts.aggregate(ALL));
    } else if (role == AGGREGATE_NOT_OK_METADATA_ROLE) {
      return QVariant::fromValue(m_dpiCounts.aggregate(NOT_OK));
    }
  } else if (ptr == &m_sizeGroupNodeId) {
    // Size group.
    const SizeGroup& group = m_sizes[index.row()];
    if (role == Qt::DisplayRole) {
      return sizeToString(group.size());
    } else if (role == AGGREGATE_METADATA_ROLE) {
      return QVariant::fromValue(group.dpiCounts().aggregate(ALL));
    } else if (role == AGGREGATE_NOT_OK_METADATA_ROLE) {
      return QVariant::fromValue(group.dpiCounts().aggregate(NOT_OK));
    }
  } else {
    // Image.
    const auto* group = static_cast<const SizeGroup*>(ptr);
    const SizeGroup::Item& item = group->items()[index.row()];
    const ImageFileInfo& file = m_files[item.fileIdx];
    if (role == Qt::DisplayRole) {
      const QString& fname = file.fileInfo().fileName();
      if (file.imageInfo().size() == 1) {
        return fname;
      } else {
        return FixDpiDialog::tr("%1 (page %2)").arg(fname).arg(item.imageIdx + 1);
      }
    } else if ((role == AGGREGATE_METADATA_ROLE) || (role == AGGREGATE_NOT_OK_METADATA_ROLE)) {
      return QVariant::fromValue(file.imageInfo()[item.imageIdx]);
    }
  }
  return QVariant();
}  // FixDpiDialog::TreeModel::data

void FixDpiDialog::TreeModel::applyDpiToAllGroups(const Scope scope, const Dpi& dpi) {
  const auto numGroups = static_cast<int>(m_sizes.size());
  for (int i = 0; i < numGroups; ++i) {
    applyDpiToGroup(scope, dpi, m_sizes[i], m_dpiCounts);
  }
}

void FixDpiDialog::TreeModel::applyDpiToGroup(const Scope scope,
                                              const Dpi& dpi,
                                              SizeGroup& group,
                                              DpiCounts& totalDpiCounts) {
  DpiCounts& groupDpiCounts = group.dpiCounts();
  const ImageMetadata metadata(group.size(), dpi);
  const std::vector<SizeGroup::Item>& items = group.items();
  const auto numItems = static_cast<int>(items.size());
  for (int i = 0; i < numItems; ++i) {
    applyDpiToItem(scope, metadata, items[i], totalDpiCounts, groupDpiCounts);
  }
}

void FixDpiDialog::TreeModel::applyDpiToItem(const Scope scope,
                                             const ImageMetadata& newMetadata,
                                             const SizeGroup::Item item,
                                             DpiCounts& totalDpiCounts,
                                             DpiCounts& groupDpiCounts) {
  ImageFileInfo& file = m_files[item.fileIdx];
  ImageMetadata& oldMetadata = file.imageInfo()[item.imageIdx];

  if ((scope == NOT_OK) && oldMetadata.isDpiOK()) {
    return;
  }

  totalDpiCounts.add(newMetadata);
  groupDpiCounts.add(newMetadata);
  totalDpiCounts.remove(oldMetadata);
  groupDpiCounts.remove(oldMetadata);

  oldMetadata = newMetadata;
}

void FixDpiDialog::TreeModel::emitAllPagesChanged(const QModelIndex& idx) {
  const auto numGroups = static_cast<int>(m_sizes.size());
  for (int i = 0; i < numGroups; ++i) {
    const QModelIndex groupNode(index(i, 0, idx));
    const int numItems = rowCount(groupNode);
    for (int j = 0; j < numItems; ++j) {
      const QModelIndex imageNode(index(j, 0, groupNode));
      emit dataChanged(imageNode, imageNode);
    }
    emit dataChanged(groupNode, groupNode);
  }

  // The 'All Pages' node.
  emit dataChanged(idx, idx);
}

void FixDpiDialog::TreeModel::emitSizeGroupChanged(const QModelIndex& idx) {
  // Every item in this size group.
  emit dataChanged(index(0, 0, idx), index(rowCount(idx), 0, idx));

  // The size group itself.
  emit dataChanged(idx, idx);

  // The 'All Pages' node.
  const QModelIndex allPagesNode(idx.parent());
  emit dataChanged(allPagesNode, allPagesNode);
}

void FixDpiDialog::TreeModel::emitItemChanged(const QModelIndex& idx) {
  // The item itself.
  emit dataChanged(idx, idx);

  // The size group node.
  const QModelIndex groupNode(idx.parent());
  emit dataChanged(groupNode, groupNode);
  // The 'All Pages' node.
  const QModelIndex allPagesNode(groupNode.parent());
  emit dataChanged(allPagesNode, allPagesNode);
}

FixDpiDialog::SizeGroup& FixDpiDialog::TreeModel::sizeGroupFor(const QSize size) {
  using namespace boost::lambda;

  const auto it(std::find_if(m_sizes.begin(), m_sizes.end(), bind(&SizeGroup::size, _1) == size));
  if (it != m_sizes.end()) {
    return *it;
  } else {
    m_sizes.emplace_back(size);
    return m_sizes.back();
  }
}

QString FixDpiDialog::TreeModel::sizeToString(const QSize size) {
  return QString("%1 x %2 px").arg(size.width()).arg(size.height());
}

/*====================== FixDpiDialog::FilterModel ======================*/

FixDpiDialog::FilterModel::FilterModel(TreeModel& delegate) : m_delegate(delegate) {
  setDynamicSortFilter(true);
  setSourceModel(delegate.model());
}

bool FixDpiDialog::FilterModel::filterAcceptsRow(const int sourceRow, const QModelIndex& sourceParent) const {
  return m_delegate.isVisibleForFilter(sourceParent, sourceRow);
}

QVariant FixDpiDialog::FilterModel::data(const QModelIndex& index, int role) const {
  if (role == AGGREGATE_METADATA_ROLE) {
    role = AGGREGATE_NOT_OK_METADATA_ROLE;
  }
  return QSortFilterProxyModel::data(index, role);
}
