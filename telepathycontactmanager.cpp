/*
 * This file is part of Kopete
 *
 * Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "telepathyprotocol.h"
#include "telepathycontactmanager.h"
#include "telepathyaccount.h"
#include "telepathycontact.h"
#include "common.h"

#include <kdebug.h>

#include <kopetemetacontact.h>
#include <kopetecontactlist.h>

#include <TelepathyQt4/Client/ContactManager>
#include <TelepathyQt4/Client/Account>
#include <TelepathyQt4/Client/PendingContacts>
#include <TelepathyQt4/Client/PendingReady>


TelepathyContactManager::TelepathyContactManager(TelepathyAccount *telepathyAccount)
{
	m_telepathyAccount = telepathyAccount;
	m_account = m_telepathyAccount->m_account;
}

TelepathyContactManager::~TelepathyContactManager()
{
}
	
QSharedPointer<Telepathy::Client::Contact> TelepathyContactManager::addContact(const QString &contactId)
{
	Q_UNUSED(contactId);
	return QSharedPointer<Telepathy::Client::Contact>();
}

void TelepathyContactManager::removeContact(TelepathyContact *contact)
{
	contact->deleteLater();
}

void TelepathyContactManager::setContactList(QList<QSharedPointer<Telepathy::Client::Contact> > contactList)
{
	Q_UNUSED(contactList);
}

void TelepathyContactManager::loadContacts()
{
}

void TelepathyContactManager::fetchContactList()
{
	kDebug(TELEPATHY_DEBUG_AREA);
	if(!m_account || !m_account->haveConnection())
	{
		kDebug(TELEPATHY_DEBUG_AREA) << "Error: Could not find active connection or account";
	}
	
	m_contactManager = m_account->connection()->contactManager();

	m_connection = m_contactManager->connection();

	QObject::connect(m_connection->becomeReady(),
		SIGNAL(finished(Telepathy::Client::PendingOperation*)),
	    this,
		SLOT(onConnectionReady(Telepathy::Client::PendingOperation*))
	);
}

void TelepathyContactManager::onConnectionReady(Telepathy::Client::PendingOperation* operation)
{
    kDebug(TELEPATHY_DEBUG_AREA);
    
    if(TelepathyCommons::isOperationError(operation))
        return;

	// \brief: Add new feature to existing connection to get contact list from server 
	Telepathy::Client::Features features = m_connection->requestedFeatures();
	features << Telepathy::Client::Connection::FeatureRoster;
	QObject::connect(m_connection->becomeReady(features),
		SIGNAL(finished(Telepathy::Client::PendingOperation*)),
		this,
		SLOT(onConnectionFeaturesReady(Telepathy::Client::PendingOperation*)));
}

void TelepathyContactManager::onConnectionFeaturesReady(Telepathy::Client::PendingOperation *operation)
{
    kDebug(TELEPATHY_DEBUG_AREA);
    
    if(TelepathyCommons::isOperationError(operation))
        return;
	
	QObject::connect(m_contactManager,
		SIGNAL(presencePublicationRequested(const Telepathy::Client::Contacts &)),
		this,
		SLOT(onPresencePublicationRequested(const Telepathy::Client::Contacts &)));

	QSet<QSharedPointer<Telepathy::Client::Contact> > contacts = m_contactManager->allKnownContacts();
	
	foreach(QSharedPointer<Telepathy::Client::Contact> contact, contacts)
	{
		kDebug(TELEPATHY_DEBUG_AREA) << contact->id() << contact->alias();
		
		Kopete::MetaContact *metaContact = new Kopete::MetaContact();
		TelepathyContact *newContact = new TelepathyContact(m_telepathyAccount, contact->id(), metaContact);
		newContact->setInternalContact(contact);
		newContact->setMetaContact(metaContact);
		
		Kopete::ContactList::self()->addMetaContact(metaContact);
	}	
}

void TelepathyContactManager::onPresencePublicationRequested(const Telepathy::Client::Contacts &contacts)
{
	foreach(QSharedPointer<Telepathy::Client::Contact> contact, contacts)
	{
		kDebug(TELEPATHY_DEBUG_AREA) << contact->id() << contact->alias();
	}	
}















