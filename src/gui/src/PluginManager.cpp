/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015 Synergy Si Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "PluginManager.h"

#include "CoreInterface.h"
#include "CommandProcess.h"
#include "DataDownloader.h"
#include "QUtility.h"
#include "ProcessorArch.h"
#include "Fingerprint.h"
#include "../lib/common/PluginVersion.h"

#include <QFile>
#include <QDir>
#include <QProcess>
#include <QCoreApplication>

static const char kBaseUrl[] = "http://synergy-project.org/files";
static const char kWinPackagePlatform32[] = "Windows-x86";
static const char kWinPackagePlatform64[] = "Windows-x64";
static const char kMacPackagePlatform[] = "MacOSX%1-i386";
static const char kLinuxPackagePlatformDeb32[] = "Linux-i686-deb";
static const char kLinuxPackagePlatformDeb64[] = "Linux-x86_64-deb";
static const char kLinuxPackagePlatformRpm32[] = "Linux-i686-rpm";
static const char kLinuxPackagePlatformRpm64[] = "Linux-x86_64-rpm";

#if defined(Q_OS_WIN)
static const char kWinPluginExt[] = ".dll";

#elif defined(Q_OS_MAC)
static const char kMacPluginPrefix[] = "lib";
static const char kMacPluginExt[] = ".dylib";
#else
static const char kLinuxPluginPrefix[] = "lib";
static const char kLinuxPluginExt[] = ".so";
#endif

PluginManager::PluginManager(QStringList pluginList) :
	m_PluginList(pluginList),
	m_DownloadIndex(-1)
{
	m_PluginDir = m_CoreInterface.getPluginDir();
	if (m_PluginDir.isEmpty()) {
		emit error(tr("Failed to get plugin directory."));
	}

	m_ProfileDir = m_CoreInterface.getProfileDir();
	if (m_ProfileDir.isEmpty()) {
		emit error(tr("Failed to get profile directory."));
	}
}

PluginManager::~PluginManager()
{
}

bool PluginManager::exist(QString name)
{
	CoreInterface coreInterface;
	QString PluginDir = coreInterface.getPluginDir();
	QString pluginName = getPluginOsSpecificName(name);
	QString filename;
	filename.append(PluginDir);
	filename.append(QDir::separator()).append(pluginName);
	QFile file(filename);
	bool exist = false;
	if (file.exists()) {
		exist = true;
	}

	return exist;
}

void PluginManager::downloadPlugins()
{
	if (m_DataDownloader.isFinished()) {
		if (!savePlugin()) {
			return;
		}

		if (m_DownloadIndex != m_PluginList.size() - 1) {
			emit downloadNext();
		}
		else {
			emit downloadFinished();
			return;
		}
	}

	m_DownloadIndex++;

	if (m_DownloadIndex < m_PluginList.size()) {
		QUrl url;
		QString pluginUrl = getPluginUrl(m_PluginList.at(m_DownloadIndex));
		if (pluginUrl.isEmpty()) {
			return;
		}
		url.setUrl(pluginUrl);

		connect(&m_DataDownloader, SIGNAL(isComplete()), this, SLOT(downloadPlugins()));

		m_DataDownloader.download(url);
	}
}

bool PluginManager::savePlugin()
{
	// create the path if not exist
	QDir dir(m_PluginDir);
	if (!dir.exists()) {
		dir.mkpath(".");
	}

	QString filename = m_PluginDir;
	QString pluginName = m_PluginList.at(m_DownloadIndex);
	pluginName = getPluginOsSpecificName(pluginName);
	filename.append(QDir::separator()).append(pluginName);

	QFile file(filename);
	if (!file.open(QIODevice::WriteOnly)) {
		emit error(
				tr("Failed to download plugin '%1' to: %2\n%3")
				.arg(m_PluginList.at(m_DownloadIndex))
				.arg(m_PluginDir)
				.arg(file.errorString()));

		file.close();
		return false;
	}

	file.write(m_DataDownloader.data());
	file.close();

	return true;
}

QString PluginManager::getPluginUrl(const QString& pluginName)
{
	QString archName;

#if defined(Q_OS_WIN)

	try {
		QString coreArch = m_CoreInterface.getArch();
		if (coreArch.startsWith("x86")) {
			archName = kWinPackagePlatform32;
		}
		else if (coreArch.startsWith("x64")) {
			archName = kWinPackagePlatform64;
		}
	}
	catch (...) {
		emit error(tr("Could not get Windows architecture type."));
		return "";
	}

#elif defined(Q_OS_MAC)

	QString macVersion = "1010";
#if __MAC_OS_X_VERSION_MIN_REQUIRED <= 1090 // 10.9
	macVersion = "109";
#elif __MAC_OS_X_VERSION_MIN_REQUIRED <= 1080 // 10.8
	macVersion = "108";
#elif __MAC_OS_X_VERSION_MIN_REQUIRED <= 1070 // 10.7
	emit error(tr("Plugins not supported on this Mac OS X version."));
	return "";
#endif

	archName = QString(kMacPackagePlatform).arg(macVersion);

#else

	QString program("dpkg");
	QStringList args;
	args << "-s" << "synergy";

	QProcess process;
	process.setReadChannel(QProcess::StandardOutput);
	process.start(program, args);
	bool success = process.waitForStarted();

	bool isDeb = (success && process.waitForFinished() & (process.exitCode() == 0));

	int arch = getProcessorArch();
	if (arch == kProcessorArchLinux32) {
		if (isDeb) {
			archName = kLinuxPackagePlatformDeb32;
		}
		else {
			archName = kLinuxPackagePlatformRpm32;
		}
	}
	else if (arch == kProcessorArchLinux64) {
		if (isDeb) {
			archName = kLinuxPackagePlatformDeb64;
		}
		else {
			archName = kLinuxPackagePlatformRpm64;
		}
	}
	else {
		emit error(tr("Could not get Linux architecture type."));
		return "";
	}

#endif

	QString result = QString("%1/plugins/%2/%3/%4/%5")
			.arg(kBaseUrl)
			.arg(pluginName)
			.arg(getExpectedPluginVersion(pluginName.toStdString().c_str()))
			.arg(archName)
			.arg(getPluginOsSpecificName(pluginName));

	qDebug() << result;
	return result;
}

QString PluginManager::getPluginOsSpecificName(const QString& pluginName)
{
	QString result = pluginName;
#if defined(Q_OS_WIN)
	result.append(kWinPluginExt);
#elif defined(Q_OS_MAC)
	result = kMacPluginPrefix + pluginName + kMacPluginExt;
#else
	result = kLinuxPluginPrefix + pluginName + kLinuxPluginExt;
#endif
	return result;
}
