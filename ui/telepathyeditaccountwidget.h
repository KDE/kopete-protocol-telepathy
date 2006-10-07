/*
 * telepathyeditaccountwidget.h - UI to edit Telepathy account settings
 *
 * Copyright (c) 2006 by Michaël Larouche <larouche@kde.org>
 * 
 * Kopete    (c) 2002-2006 by the Kopete developers  <kopete-devel@kde.org>
 *
 *************************************************************************
 *                                                                       *
 * This program is free software; you can redistribute it and/or modify  *
 * it under the terms of the GNU General Public License as published by  *
 * the Free Software Foundation; either version 2 of the License, or     *
 * (at your option) any later version.                                   *
 *                                                                       *
 *************************************************************************
 */
#ifndef TELEPATHYEDITACCOUNTWIDGET_H
#define TELEPATHYEDITACCOUNTWIDGET_H

#include <QtGui/QWidget>
#include <editaccountwidget.h>

namespace Ui
{
	class TelepathyEditAccountWidget;
}

namespace Kopete
{
	class Account;
}

class QTreeWidgetItem;
class TelepathyAccount;
/**
 * @brief Edit Telepathy account settings.
 * @author Michaël Larouche <larouche@kde.org>
 */
class TelepathyEditAccountWidget : public QWidget, public KopeteEditAccountWidget
{
	Q_OBJECT
public:
	TelepathyEditAccountWidget(Kopete::Account *account, QWidget *parent = 0);
	~TelepathyEditAccountWidget();

	virtual bool validateData();

	/**
	 * Create a new account if we are in the 'add account wizard',
	 * otherwise update the existing account.
	 */
	virtual Kopete::Account *apply();

protected:
	/**
	 * @brief Reimplement account() to access TelepathyAccount specific methods.
	 */
	TelepathyAccount *account();

private slots:
	void connectionManagerSelectionChanged();
	void protocolSelectionChanged();
	void listConnectionManager();
	void readConfig();
	void writeConfig();

private:
	class Private;
	Private *d;
};

#endif
