#ifndef SRC_INFRA_CRASH_REPORTER_HPP_
#define SRC_INFRA_CRASH_REPORTER_HPP_

#include "base/FileSystem.hpp"

#include <memory>
#include <string>

namespace crashpad {
class CrashpadClient;
class CrashReportDatabase;
}

namespace scin { namespace infra {

/*! Uses Crashpad to launch an out-of-process crash monitoring executable, which can collect forensic information
 * automatically on detection of a crash or also upon request. Crashes are stored in a local database and can be
 * uploaded to the crash telemetry server, Gargamelle.
 */
class CrashReporter {
public:
    /*! CrashReporter constructor.
     *
     * \param handler The path to the executable to run as the crashpad handler.
     * \param database A path to the crash report database root directory. If it does not exist it will be created.
     * \param metrics A path to the metrics database root directory. If it does not exist it will be created.
     */
    CrashReporter(const fs::path& handler, const fs::path& database, const fs::path& metrics);

    /*! CrashReporter destructor. Will disable crash reporting when executed, so should be deferred as late as possible
     * in process exit.
     */
    ~CrashReporter();

    /*! Start the external crash handling process. Blocks until it has been successfully started.
     *
     * \return true if the hander was successfully started.
     */
    bool startCrashHandler();

    /*! Opens the crash report database. Any crash report query or settings change will open the database automatically,
     * but it can be opened and closed manually to allow for efficiency with multiple queries.
     *
     * \return true on success, false on failure.
     */
    bool openDatabase();

    /*! Closes the crash report database. It is not required to have the database open to collect crash reports.
     */
    void closeDatabase();

    /*! Print all crash reports in the database to the log at the informational level (2).
     *
     * \return The number of un-uploaded crash reports in the database, or -1 if there was an error.
     */
    int logCrashReports();

    /*! Mark the provided crash report as ready for upload by the handler process.
     *
     * \param reuprtUUID The UUID of the crash report to upload. These can be extracted from the log.
     * \return true if the crash report was found and successfully marked for upload, false otherwise.
     */
    bool uploadCrashReport(const std::string& reportUUID);

    /*! Mark all unuploaded crash reports as ready for upload by the handler process.
     *
     * \return true if all reports were successfully marked, false otherwise.
     */
    bool uploadAllCrashReports();

private:
    fs::path m_handlerPath;
    fs::path m_databasePath;
    fs::path m_metricsPath;

    std::unique_ptr<crashpad::CrashReportDatabase> m_database;
    std::unique_ptr<crashpad::CrashpadClient> m_client;
};

} // namespace infra
} // namespace scin

#endif // SRC_INFRA_CRASH_REPORTER_HPP_
