#ifndef _MACHINELISTMODEL_H_
#define _MACHINELISTMODEL_H_

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <QList>
#include <QString>
#include <QVariant>
#include <QIcon>
#include <QSqlQuery>

#include "macros.h"

class MachineListModelItem
{
	public:
		MachineListModelItem(const QString &id, const QIcon &icon, const QString &parent, const QString &description, const QString &manufacturer, const QString &year, const QString &source_file, int players, const QString &category, const QString &version, int rank, char rom_status, bool has_roms, bool has_chds, const QString &driver_status, bool is_device, bool is_bios, bool tagged, MachineListModelItem *parentItem = 0);
		MachineListModelItem(MachineListModelItem *parentItem = 0);
		~MachineListModelItem();

		void setId(const QString &id) { m_id = id; }
		QString &id() { return m_id; }
		void setParent(const QString &parent) { m_parent = parent; }
		QString &parent() { return m_parent; }
		void setDescription(const QString &description) { m_description = description; }
		QString &description() { return m_description; }
		void setManufacturer(const QString &manufacturer) { m_manufacturer = manufacturer; }
		QString &manufacturer() { return m_manufacturer; }
		void setYear(const QString &year) { m_year = year; }
		QString &year() { return m_year; }
		void setSourceFile(const QString &source_file) { m_source_file = source_file; }
		QString &sourceFile() { return m_source_file; }
		void setPlayers(int players) { m_players = players; }
		int players() { return m_players; }
		void setCategory(const QString &category) { m_category = category; }
		QString &category() { return m_category; }
		void setVersion(const QString &version) { m_version = version; }
		QString &version() { return m_version; }
		void setIcon(const QIcon &icon) { m_icon = icon; }
		QIcon &icon() { return m_icon; }
		void setRank(int rank) { m_rank = rank; }
		int rank() { return m_rank; }
		void setRomStatus(char rom_status) { m_rom_status = rom_status; }
		char romStatus() { return m_rom_status; }
		void setHasRoms(bool has_roms) { m_has_roms = has_roms; }
		bool hasRoms() { return m_has_roms; }
		void setHasChds(bool has_chds) { m_has_chds = has_chds; }
		bool hasChds() { return m_has_chds; }
		void setDriverStatus(const QString &driver_status) { m_driver_status = driver_status; }
		QString &driverStatus() { return m_driver_status; }
		void setIsDevice(bool is_device) { m_is_device = is_device; }
		bool isDevice() { return m_is_device; }
		void setIsBios(bool is_bios) { m_is_bios = is_bios; }
		bool isBios() { return m_is_bios; }
		void setTag(bool tagged) { m_tagged = tagged; }
		bool tagged() { return m_tagged; }

		void setParentItem(MachineListModelItem *item) { m_parentItem = item; }
		MachineListModelItem *parentItem() { return m_parentItem; }
		QList<MachineListModelItem *> &childItems() { return m_childItems; }
		int row() const;

	private:
		QString m_id;
		QString m_parent;
		QString m_description;
		QString m_manufacturer;
		QString m_year;
		QString m_source_file;
		QString m_category;
		QString m_version;
		QString m_driver_status;
		QIcon m_icon;
		int m_players;
		int m_rank;
		char m_rom_status;
		bool m_has_roms;
		bool m_has_chds;
		bool m_is_device;
		bool m_is_bios;
		bool m_tagged;
		QList<MachineListModelItem *> m_childItems;
		MachineListModelItem *m_parentItem;
};

class MachineListProxyModel : public QSortFilterProxyModel
{
	Q_OBJECT

	public:
		MachineListProxyModel(QObject *parent = 0);

	protected:
		QVariant headerData(int section, Qt::Orientation orientation, int role) const;
		bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;
		bool lessThan(const QModelIndex &left, const QModelIndex &right) const;
		int rowCount(const QModelIndex &parent = QModelIndex()) const;

	private:
};

class MachineListModel : public QAbstractItemModel
{
	Q_OBJECT

	public:
		enum Column {TAG, ICON, ID, PARENT, DESCRIPTION, MANUFACTURER, YEAR, ROM_STATUS, HAS_ROMS, HAS_CHDS, DRIVER_STATUS, SOURCE_FILE, PLAYERS, RANK, IS_BIOS, IS_DEVICE, CATEGORY, VERSION, LAST_COLUMN};

		MachineListModel(QObject *parent = 0);
		~MachineListModel();

		void setRootItem(MachineListModelItem *item);
 
		virtual QModelIndex index(int row, int column, const QModelIndex &parent) const;
		virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
		virtual bool canFetchMore(const QModelIndex &parent) const;
		virtual void fetchMore(const QModelIndex &parent);
		virtual Qt::ItemFlags flags(const QModelIndex &index) const;
		virtual int columnCount(const QModelIndex &) const;
		virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
		virtual QVariant data(const QModelIndex &index, int role) const;
		virtual QModelIndex parent(const QModelIndex &child) const;

	public slots:
		void startQuery();

	private:
		QStringList m_headers;
		MachineListModelItem *m_rootItem;
		QSqlQuery *m_query;
		qint64 m_recordCount;

		MachineListModelItem *itemFromIndex(const QModelIndex &index) const;
};

#endif
