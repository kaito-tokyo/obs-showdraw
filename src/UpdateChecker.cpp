/*
obs-showdraw
Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include "UpdateChecker.hpp"

#include <QNetworkReply>
#include <QDebug>

UpdateChecker::UpdateChecker(QObject *parent) : QObject(parent)
{
	manager = new QNetworkAccessManager();
}

void UpdateChecker::fetch(void)
{
	QNetworkRequest request(QUrl("https://api.github.com/repos/kaito-tokyo/obs-showdraw/releases/latest"));
	QNetworkReply *reply = manager->get(request);
	connect(reply, &QNetworkReply::finished, this, [this, reply]() {
		if (reply->error() != QNetworkReply::NoError) {
			// Handle error
			reply->deleteLater();
            throw std::runtime_error("Network error occurred while fetching update information.");
			return;
		}

		QByteArray responseData = reply->readAll();
		qDebug() << "Response Data:" << responseData;

		reply->deleteLater();
	});
}

void UpdateChecker::isUpdateAvailable(void) const noexcept {}
