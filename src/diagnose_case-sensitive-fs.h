#pragma once

#include <QString>
#include <uibase/iplugin.h>
#include <uibase/iplugindiagnose.h>

#define EXPORT __attribute__((visibility("default")))

namespace MOBase
{
class IModInterface;
}

/**
 * @brief A diagnostic plugin for case-sensitive file systems
 */
class EXPORT DiagnoseCaseSensitiveFS final : public QObject,
                                             public MOBase::IPlugin,
                                             public MOBase::IPluginDiagnose
{
  Q_OBJECT
  Q_INTERFACES(MOBase::IPlugin MOBase::IPluginDiagnose)
  Q_PLUGIN_METADATA(IID "org.Kaedras.DiagnoseCaseSensitiveFS")

public:
  DiagnoseCaseSensitiveFS();
  ~DiagnoseCaseSensitiveFS() override;

  bool init(MOBase::IOrganizer* organizer) override;
  [[nodiscard]] QString name() const override;
  [[nodiscard]] QString author() const override;
  [[nodiscard]] QString description() const override;
  [[nodiscard]] MOBase::VersionInfo version() const override;
  [[nodiscard]] QList<MOBase::PluginSetting> settings() const override;

  // IPluginDiagnose
  [[nodiscard]] std::vector<unsigned int> activeProblems() const override;
  [[nodiscard]] QString shortDescription(unsigned int key) const override;
  [[nodiscard]] QString fullDescription(unsigned int key) const override;
  [[nodiscard]] bool hasGuidedFix(unsigned int key) const override;
  void startGuidedFix(unsigned int key) const override;

private:
  static constexpr unsigned int PROBLEM_INCONSISTENT_CAPITALIZATION = 1;

  MOBase::IOrganizer* m_organizer;

  [[nodiscard]] bool hasInconsistentPaths() const noexcept;

  void fixInconsistentPaths() const noexcept;

  /**
   * @brief Renames the mod paths to lower case, excluding paths that exist in the game
   * data directory.
   * @param mod Mod to process.
   */
  void renameModPathsToLowerCase(const MOBase::IModInterface* mod) const noexcept;

  /**
   * @brief Renames the next entry inside the provided path that requires
   * renaming. This function is called by @link renameModPathsToLowerCase @endlink.
   * It is required because QDirIterator becomes invalid after renaming a directory.
   */
  [[nodiscard]] bool renameNext(const QString& path) const noexcept;

  /**
   * @brief Checks whether the provided file or directory exists
   * (case-insensitive).
   * Case insensitivity only applies to the file name, not the
   * parent path.
   * @param path Path to check.
   * @return Whether the path exists
   * or not.
   */
  static bool existsCaseInsensitive(const QString& path) noexcept;

  /**
   * @brief Searches the parent directory of the provided path for filename. Case
   * insensitivity only applies to the file name, not the parent path.
   */
  static QString getFileNameCaseInsensitive(const QString& path) noexcept;
};
