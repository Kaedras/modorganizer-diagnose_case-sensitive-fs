#include "diagnose_case-sensitive-fs.h"

#include <QDirIterator>
#include <iostream>
#include <uibase/ifiletree.h>
#include <uibase/imoinfo.h>
#include <uibase/iplugingame.h>
#include <uibase/ipluginlist.h>
#include <uibase/log.h>

using namespace MOBase;
using namespace std;
using namespace Qt::StringLiterals;

DiagnoseCaseSensitiveFS::DiagnoseCaseSensitiveFS() : m_organizer(nullptr) {}

DiagnoseCaseSensitiveFS::~DiagnoseCaseSensitiveFS() = default;

bool DiagnoseCaseSensitiveFS::init(IOrganizer* organizer)
{
  m_organizer = organizer;
  m_organizer->modList()->onModInstalled([&](const IModInterface* modInterface) {
    if (m_organizer->pluginSetting(name(), u"auto_rename_to_lower_case"_s).toBool()) {
      renameModPathsToLowerCase(modInterface);
    }
  });
  m_organizer->onAboutToRun([&](const QString&) {
    if (m_organizer->pluginSetting(name(), u"prevent_launch"_s).toBool() &&
        hasInconsistentPaths()) {
      log::info("{} aborted application launch because there are unresolved issues. "
                "You can disable this behaviour in plugin settings.",
                name());
      return false;
    }
    return true;
  });
  return true;
}

QList<PluginSetting> DiagnoseCaseSensitiveFS::settings() const
{
  return {
      {u"prevent_launch"_s, tr("Prevent application launch if issues are found"), true},
      {u"auto_rename_to_lower_case"_s,
       tr("Automatically rename paths to lower case on mod installation (excluding "
          "paths that exist inside game directory)"),
       true},
  };
}

std::vector<unsigned int> DiagnoseCaseSensitiveFS::activeProblems() const
{
  std::vector<unsigned int> result;
  if (hasInconsistentPaths()) {
    result.push_back(PROBLEM_INCONSISTENTCAPITALIZATION);
  }
  return result;
}

QString DiagnoseCaseSensitiveFS::shortDescription(unsigned int key) const
{
  if (key == PROBLEM_INCONSISTENTCAPITALIZATION) {
    return tr("Mods contain inconsistent paths");
  }
  throw Exception(tr("Invalid problem key %1").arg(key));
}

QString DiagnoseCaseSensitiveFS::fullDescription(unsigned int key) const
{
  if (key == PROBLEM_INCONSISTENTCAPITALIZATION) {
    return tr("The installed mods use different capitalization for file paths that are "
              "otherwise identical. This will most likely cause issues on case "
              "sensitive file systems.");
  }
  throw Exception(tr("Invalid problem key %1").arg(key));
}

bool DiagnoseCaseSensitiveFS::hasGuidedFix(unsigned int key) const
{
  if (key == PROBLEM_INCONSISTENTCAPITALIZATION) {
    return true;
  }
  throw Exception(tr("Invalid problem key %1").arg(key));
}

void DiagnoseCaseSensitiveFS::startGuidedFix(unsigned int key) const
{
  if (key == PROBLEM_INCONSISTENTCAPITALIZATION) {
    fixInconsistentPaths();
  } else {
    throw Exception(tr("Invalid problem key %1").arg(key));
  }
}

QString DiagnoseCaseSensitiveFS::name() const
{
  return u"Diagnosis plugin for case sensitive filesystems"_s;
}

QString DiagnoseCaseSensitiveFS::author() const
{
  return u"Kaedras"_s;
}

QString DiagnoseCaseSensitiveFS::description() const
{
  return u"Diagnostic plugin for case-sensitive file systems."_s;
}

bool DiagnoseCaseSensitiveFS::hasInconsistentPaths() const noexcept
{
  TimeThis tt(u"hasInconsistentPaths()"_s);
  QMap<QString, QString> relativePathMap;

  // check mods
  IModList* modList = m_organizer->modList();
  QStringList mods  = modList->allMods();
  for (const auto& mod : mods) {
    IModInterface* modInfo = modList->getMod(mod);
    QString modPath        = modInfo->absolutePath();
    QDirIterator iter(modPath, QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files,
                      QDirIterator::Subdirectories);
    while (iter.hasNext()) {
      const QString absolutePath = iter.next();
      const QString relativePath = absolutePath.sliced(modPath.length() + 1);
      if (relativePathMap.contains(relativePath.toLower())) {
        if (relativePathMap[relativePath.toLower()] != relativePath) {
          return true;
        }
      } else {
        relativePathMap.insert(relativePath.toLower(), relativePath);
      }
    }
  }

  // check game data
  const QString gameDataPath =
      m_organizer->managedGame()->dataDirectory().absolutePath();
  QDirIterator iter(gameDataPath, QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files,
                    QDirIterator::Subdirectories);
  while (iter.hasNext()) {
    const QString absolutePath = iter.next();
    const QString relativePath = absolutePath.sliced(gameDataPath.length() + 1);

    if (relativePathMap.contains(relativePath.toLower())) {
      if (relativePathMap[relativePath.toLower()] != relativePath) {
        return true;
      }
    } else {
      relativePathMap.insert(relativePath.toLower(), relativePath);
    }
  }

  return false;
}

void DiagnoseCaseSensitiveFS::fixInconsistentPaths() const noexcept
{
  IModList* modList     = m_organizer->modList();
  QStringList modsNames = modList->allMods();
  for (const QString& modName : modsNames) {
    renameModPathsToLowerCase(modList->getMod(modName));
  }
}

bool DiagnoseCaseSensitiveFS::renameNext(
    const QString& path, QFlags<QDir::Filters::enum_type> whatToRename) const noexcept
{
  QDirIterator iter(path, whatToRename | QDir::NoDotAndDotDot,
                    QDirIterator::Subdirectories);
  while (iter.hasNext()) {
    QFileInfo entry = iter.nextFileInfo();

    QString absoluteFilePath = entry.absoluteFilePath();
    QString absoluteWithLowerCaseFileName =
        entry.absolutePath() % u"/"_s % entry.fileName().toLower();
    QString relativeFilePath =
        entry.absoluteFilePath().sliced(path.size() + 1);  // +1 to include slash
    QString gameFilePath = m_organizer->managedGame()->dataDirectory().absolutePath() %
                           u"/"_s % relativeFilePath;

    // rename to lower case if entry does not exist in game path
    if (!existsCaseInsensitive(gameFilePath)) {
      // skip entry it's already lower case
      if (absoluteFilePath == absoluteWithLowerCaseFileName) {
        continue;
      }
      bool result = QFile::rename(absoluteFilePath, absoluteWithLowerCaseFileName);
      if (!result) {
        const int e = errno;
        log::error("Error renaming {} to {}, {}", absoluteFilePath,
                   absoluteWithLowerCaseFileName, strerror(e));
        return false;
      }
      return true;
    }
    // rename to match file in game path
    if (!QFile::exists(gameFilePath)) {
      QString targetFileName = getFileNameCaseInsensitive(gameFilePath);
      if (targetFileName.isEmpty()) {
        log::warn("Error getting matching filename in game data directory, path was {}",
                  gameFilePath);
        return false;
      }
      QString sourceFileName = QFileInfo(absoluteFilePath).fileName();
      // skip entry if path is correct
      if (sourceFileName == targetFileName) {
        continue;
      }

      QFile::rename(absoluteFilePath, entry.absolutePath() % u"/"_s % targetFileName);
      return true;
    }
  }
  return false;
}

void DiagnoseCaseSensitiveFS::renameModPathsToLowerCase(
    const IModInterface* mod,
    QFlags<QDir::Filters::enum_type> whatToRename) const noexcept
{
  TimeThis tt(u"renameModPathsToLowerCase(%1)"_s.arg(mod->name()));
  if (mod == nullptr) {
    log::warn("renameModPathsToLowerCase: parameter is nullptr");
    return;
  }

  QString modPath = mod->absolutePath();
  // Skip mod if it's located inside game data directory, this can happen with DLC
  if (modPath == m_organizer->managedGame()->dataDirectory().absolutePath()) {
    return;
  }
  while (renameNext(modPath, whatToRename)) {
    // Repeat until function returns false
    // This is required because QDirIterator becomes invalid after renaming a directory
  }
}

bool DiagnoseCaseSensitiveFS::existsCaseInsensitive(const QString& path) noexcept
{
  QFileInfo info(path);
  QString fileName = info.fileName();
  QDirIterator iter(info.dir());
  while (iter.hasNext()) {
    if (iter.nextFileInfo().fileName().compare(fileName, Qt::CaseInsensitive) == 0) {
      return true;
    }
  }

  return false;
}

QString
DiagnoseCaseSensitiveFS::getFileNameCaseInsensitive(const QString& path) noexcept
{
  QFileInfo info(path);
  QString fileName = info.fileName();
  QDirIterator iter(info.dir());
  while (iter.hasNext()) {
    QString nextFileName = iter.nextFileInfo().fileName();
    if (nextFileName.compare(fileName, Qt::CaseInsensitive) == 0) {
      return nextFileName;
    }
  }
  return {};
}

VersionInfo DiagnoseCaseSensitiveFS::version() const
{
  return {1, 0, 0, VersionInfo::RELEASE_FINAL};
}