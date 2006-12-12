/*
 * telepathycontact.cpp - Telepathy Kopete Contact.
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
#include "telepathycontact.h"

// Qt includes
#include <QtCore/QPointer>
#include <QtGui/QImage>

// KDE includes
#include <kaction.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>

// Kopete includes
#include <kopetechatsessionmanager.h>
#include <kopetemetacontact.h>
#include <kopeteuiglobal.h>

// QtTapioca includes
#include <QtTapioca/Contact>
#include <QtTapioca/TextChannel>
#include <QtTapioca/Avatar>

// Telepathy includes
#include "telepathyaccount.h"
#include "telepathyprotocol.h"
#include "telepathycontactmanager.h"
#include "telepathychatsession.h"

using namespace QtTapioca;

class TelepathyContact::Private
{
public:
	Private()
	{}

	QPointer<QtTapioca::Contact> internalContact;
	QPointer<Kopete::ChatSession> currentChatSession;
};

TelepathyContact::TelepathyContact(TelepathyAccount *account, const QString &contactId, Kopete::MetaContact *parent)
 : Kopete::Contact(account, contactId, parent), d(new Private)
{
}

TelepathyContact::~TelepathyContact()
{
	delete d;
}

TelepathyAccount *TelepathyContact::account()
{
	return static_cast<TelepathyAccount*>( Kopete::Contact::account() );
}

QtTapioca::Contact *TelepathyContact::internalContact()
{
// 	Q_ASSERT( !d->internalContact.isNull() );
	return d->internalContact;
}

void TelepathyContact::setInternalContact(QtTapioca::Contact *internalContact)
{
	kDebug(TELEPATHY_DEBUG_AREA) << k_funcinfo << "Updating internal contact pointer for " << contactId() << endl;

	if( !d->internalContact.isNull() )
	{
		// Disconnect signals from previous internal contact
		d->internalContact->disconnect();
	}
	d->internalContact = internalContact;

	// Connect signal/slots
	connect(d->internalContact, SIGNAL(presenceUpdated(QtTapioca::ContactBase*, QtTapioca::ContactBase::Presence, QString)), this, SLOT(telepathyPresenceUpdated(QtTapioca::ContactBase*, QtTapioca::ContactBase::Presence, QString)));
	connect(d->internalContact, SIGNAL(aliasChanged(QtTapioca::ContactBase*,QString)), this, SLOT(telepathyAliasChanged(QtTapioca::ContactBase*,QString)));
	connect(d->internalContact, SIGNAL(avatarUpdated(QtTapioca::ContactBase *)), this, SLOT(telepathyAvatarChanged(QtTapioca::ContactBase*)));

	// Set initial presence
	TelepathyProtocol::protocol()->telepathyStatusToKopete( d->internalContact->presence() );

	// Set nickname/alias
	setNickName( d->internalContact->alias() );

	// Enable avatar update
	d->internalContact->setReceiveAvatarUpdates( true, property(TelepathyProtocol::protocol()->propAvatarToken).value().toString() );
	telepathyAvatarChanged( d->internalContact );
}

bool TelepathyContact::isReachable()
{
	return account()->isConnected();
}

void TelepathyContact::serialize(QMap< QString, QString >& serializedData, QMap< QString, QString >& addressBookData)
{
	Q_UNUSED(serializedData);
	Q_UNUSED(addressBookData);
	// Nothing specific to serialize yet.
}

QList<KAction *> *TelepathyContact::customContextMenuActions()
{
	return 0;
}

Kopete::ChatSession *TelepathyContact::manager(CanCreateFlags canCreate)
{
	if( d->currentChatSession.isNull() )
	{
		QList<Kopete::Contact*> others;
		others.append( this );

		// Fist try to find an existing chat session
		Kopete::ChatSession *existingSession = Kopete::ChatSessionManager::self()->findChatSession( account()->myself(), others, account()->protocol() );
		if( existingSession )
		{
			d->currentChatSession = existingSession;
		}
		// Else create a new chat session and text channel
		else if( canCreate == Kopete::Contact::CanCreate )
		{
			TelepathyChatSession *newSession = new TelepathyChatSession( account()->myself(), others, account()->protocol() );
			// Assume that we create a new session
			TextChannel *textChannel = account()->createTextChannel( internalContact() );
			if( textChannel )
			{
				newSession->setTextChannel(textChannel);
				d->currentChatSession = newSession;
			}
		}
	}

	return d->currentChatSession;
}

void TelepathyContact::deleteContact()
{
	if( !account()->isConnected() )
	{
		KMessageBox::queuedMessageBox( Kopete::UI::Global::mainWidget(), KMessageBox::Error, i18n("You must be connected to delete a contact."), i18n("Telepathy plugin") );
		return;
	}

	account()->contactManager()->removeContact(this);
}

void TelepathyContact::telepathyPresenceUpdated(QtTapioca::ContactBase *contactBase, QtTapioca::ContactBase::Presence presence, const QString &presenceMessage)
{
	Q_UNUSED(contactBase);

	Kopete::OnlineStatus newStatus = TelepathyProtocol::protocol()->telepathyStatusToKopete(presence);

	kDebug(TELEPATHY_DEBUG_AREA) << k_funcinfo << "Updating " << contactId() << " presence to " << newStatus.description() << endl;
	kDebug(TELEPATHY_DEBUG_AREA) << k_funcinfo << "New Status Message for " << contactId() << ": " << presenceMessage << endl;

	setOnlineStatus( newStatus );
	setStatusMessage( Kopete::StatusMessage(presenceMessage) );
}

void TelepathyContact::telepathyAliasChanged(QtTapioca::ContactBase *contactBase, const QString &newAlias)
{
	Q_UNUSED(contactBase);

	kDebug(TELEPATHY_DEBUG_AREA) << k_funcinfo << "Changing " << contactId() << " alias to " << newAlias << endl;

	setNickName( newAlias );
}

void TelepathyContact::telepathyAvatarChanged(QtTapioca::ContactBase *contactBase)
{
	if( contactBase->avatar()/* && !contactBase->avatar()->image().isEmpty() */)
	{
		kDebug(TELEPATHY_DEBUG_AREA) << k_funcinfo << "Got a avatar update for " << contactId() << endl;

		// TODO: Use a common avatar storage for all protocols
		QString pictureLocation = KStandardDirs::locateLocal( "appdata", "telepathypictures/" + contactId().replace(QRegExp("[./~]"),"-")  + ".png" );

		if( contactBase->avatar()->image().isEmpty() )
			kDebug(TELEPATHY_DEBUG_AREA) << k_funcinfo << "Whoopies, avatar image is empty" << endl;

		// Guess file format from header for now
		QImage avatar = QImage::fromData( contactBase->avatar()->image() );

		if( avatar.save(pictureLocation, "PNG") )
		{
			kDebug(TELEPATHY_DEBUG_AREA) << k_funcinfo << "Setting avatar information for " << contactId() << endl;

			// Set avatar in Kopete
			setProperty( Kopete::Global::Properties::self()->photo(), pictureLocation );
			setProperty( TelepathyProtocol::protocol()->propAvatarToken, contactBase->avatar()->token() );
		}
		else
		{
			kDebug(TELEPATHY_DEBUG_AREA) << k_funcinfo << "Removing avatar information for " << contactId() << endl;

			removeProperty( Kopete::Global::Properties::self()->photo() );
			removeProperty( TelepathyProtocol::protocol()->propAvatarToken );
		}
	}
}

#include "telepathycontact.moc"
