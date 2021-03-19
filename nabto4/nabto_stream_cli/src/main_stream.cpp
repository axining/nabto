/**
 * @file Nabto Client
 *
 * @mainpage Introduction
 *
 * @author yuanxin
 * ---------------------------------------------------------------------------
 * The Nabto Stream Client 
 * ---------------------------------------------------------------------------
 */
#include <nabto_client_api.h>
#include <nabto_stream_log.h>
#include <cxxopts.hpp>

#include <iostream>
#include <fstream>
#include <thread>
#include <string>
#include <memory>
#include <chrono>
#include <iomanip>
#include <time.h>
#include<unistd.h>
#include <mutex>

#define NABTO_STREAM_RECEIVE_WINDOW_SIZE 100

const std::string eofSequence = "Ctrl+d"; /*linux*/

bool sendFlag = 0;

pthread_mutex_t streamLock;

void die(const std::string& msg, int status=1) {
    std::cout << msg << std::endl;
    nabtoShutdown();
    exit(status);
}

void help(cxxopts::Options& options) {
    std::cout << options.help({"", "Group"}) << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
// show stuff

bool showLocalDevices() {
    char** devices;
    int devicesLength;
    nabto_status_t status;

    status = nabtoGetLocalDevices(&devices, &devicesLength);
    if (status != NABTO_OK) {
        return false;
    }

    for(int i = 0; i < devicesLength; i++) {
        std::cout << devices[i] << std::endl;
    }

    for (int i = 0; i < devicesLength; i++) {
        nabtoFree(devices[i]);
    }
    nabtoFree(devices);
    return true;
}

bool showVersion() {
    char* version;
    nabto_status_t status = nabtoVersionString(&version);
    std::cout << version << std::endl;
    nabtoFree(version);
    return status == NABTO_OK;
}

////////////////////////////////////////////////////////////////////////////////
//nabto_init

bool init(cxxopts::Options& options) {
    nabto_status_t st;
    if (options.count("home-dir")) {
        st = nabtoStartup(options["home-dir"].as<std::string>().c_str());
    } else {
        st = nabtoStartup(NULL);
    }
    return st == NABTO_OK && nabtoInstallDefaultStaticResources(NULL) == NABTO_OK;
}

////////////////////////////////////////////////////////////////////////////////
//cert

bool getFingerprintString(const std::string& commonName, std::string& fingerprintString) {
    char fingerprint[16];
    nabto_status_t st = nabtoGetFingerprint(commonName.c_str(), fingerprint);
    if (st != NABTO_OK) {
        return false;
    }
    char buf[3*sizeof(fingerprint)];
    for (size_t i=0; i<sizeof(fingerprint)-1; i++) {
        sprintf(buf+3*i, "%02x:", (unsigned char)(fingerprint[i]));
    }
    sprintf(buf+3*15, "%02x", (unsigned char)(fingerprint[15]));
    fingerprintString = std::string(buf);
    return true;

}

bool certCreate(const std::string& commonName, const std::string& password) {
    if ( password.compare("not-so-secret") == 0 ){
        std::cout << "Warning: creating certificate with default password for user: " << commonName << std::endl;
    }
    nabto_status_t st = nabtoCreateSelfSignedProfile(commonName.c_str(), password.c_str());
    if (st != NABTO_OK) {
        std::cout << "Failed to create self signed certificate " << st << std::endl;
        return false;
    }
    std::string fingerprint;
    if (getFingerprintString(commonName, fingerprint)) {
        std::cout << "Created self signed cert with fingerprint [" << fingerprint << "]" << std::endl;
        return true;
    } else {
        std::cout << "Failed to get fingerprint of self signed certificate " << st << std::endl;
        return false;
    }
}

bool certList() {
    char** certificates;
    int certificatesLength;
    nabto_status_t status;

    status = nabtoGetCertificates(&certificates, &certificatesLength);
    if (status != NABTO_OK) {
        fprintf(stderr, "nabtoGetCertificates failed: %d (%s)\n", (int) status, nabtoStatusStr(status));
        return false;
    }

    for (int i = 0; i < certificatesLength; i++) {
        std::string fingerprint;
        if (getFingerprintString(certificates[i], fingerprint)) {
            std::cout << fingerprint << " " << certificates[i] << std::endl;
        }
    }

    for (int i = 0; i < certificatesLength; i++) {
        nabtoFree(certificates[i]);
    }
    nabtoFree(certificates);
    return true;
}

bool certOpenSession(nabto_handle_t& session, cxxopts::Options& options) {
    const std::string& cert = options["cert-name"].as<std::string>();
    const std::string& passwd = options["password"].as<std::string>();
    nabto_status_t status = nabtoOpenSession(&session, cert.c_str(), passwd.c_str());
    if (status == NABTO_OK) {
        if (options.count("bs-auth-json")) {
            const std::string& json = options["bs-auth-json"].as<std::string>();
            if (nabtoSetBasestationAuthJson(session, json.c_str()) == NABTO_OK) {
                return true;
            } else {
                std::cout << "Cert opened ok, but could not set basestation auth json doc" << std::endl;
                return false;
            }
        }
        return true;
    } else if (status == NABTO_OPEN_CERT_OR_PK_FAILED) {
        std::cout << "No such certificate " << cert << std::endl;
    } else if (status == NABTO_UNLOCK_PK_FAILED) {
        std::cout << "Invalid password specified for " << cert << std::endl;
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////
//stream read data
std::mutex iomutex_;
void readData(nabto_stream_t stream, size_t mode)
{
    if (mode == 0) {
        size_t totalBytes = 0, usedTime;
        time_t start, end;
        start = time(NULL);
        for (;;) {
            size_t size;
            char* response;
            end = time(NULL);
         //   std::lock_guard<std::mutex> lock(iomutex_);
         //   pthread_mutex_lock(&(streamLock));
            nabto_status_t status = nabtoStreamRead(stream, &response, &size);
         //   pthread_mutex_unlock(&(streamLock));
            if (status != NABTO_OK) {
                NABTO_LOG_TRACE("nabtoStreamRead failed with status = %u!", status);
                break;
            }
            totalBytes = totalBytes + size;
            NABTO_LOG_TRACE("totalBytes = %d", totalBytes);
        }
    //    end = time(NULL);
        usedTime = (size_t)(end-start);
        NABTO_LOG_TRACE("revTimeSt = %ld, revTimeEd = %ld", start, end);
        NABTO_LOG_TRACE("usedTime = %ds", usedTime);
        NABTO_LOG_TRACE("You have received %d Bytes", totalBytes);
        NABTO_LOG_TRACE("stream received rate is %.2f KBps", 1.0*totalBytes/usedTime/1024);
    } else if (mode == 1) {
        size_t totalBytes = 0;
        for (;;) {
            size_t size;
            char* response;
            nabto_status_t status = nabtoStreamRead(stream, &response, &size);
            if (status != NABTO_OK) {
                NABTO_LOG_TRACE("nabtoStreamRead failed with status = %u!", status);
                break;
            }
            totalBytes = totalBytes + size;
            NABTO_LOG_TRACE("received %u bytes with status = %d", size, status);
            NABTO_LOG_TRACE("Received stream data: %s", response);
            nabtoFree(response);
        }
        NABTO_LOG_TRACE("totalBytes = %d", totalBytes);
    } else {
        die("nabto stream read mode error", 0);
    }
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{


    try
    {
        cxxopts::Options options(argv[0], "Nabto stream echo client !");
        options.add_options()
            ("c,create-cert", "Create self signed certificate")
            ("n,cert-name", "Certificate name. ex.: nabto-user", cxxopts::value<std::string>())
            ("a,password", "Password for encrypting private key", cxxopts::value<std::string>()->default_value("not-so-secret"))
            ("d,deviceid", "Device ID to connect to", cxxopts::value<std::string>())
            ("p,productid", "Product ID to use", cxxopts::value<std::string>())
            ("k,serverkey", "Server key of the app", cxxopts::value<std::string>())
            ("H,home-dir", "Override default Nabto home directory. ex.: /path/to/dir", cxxopts::value<std::string>())
            ("discover", "Show Nabto devices ids discovered on local network")
            ("certs", "Show available certificates")
            ("v,version", "Show version")
            ("h,help", "Shows this help text");
        options.parse(argc, argv);

        if (!init(options)) {
            die("Initialization failed");
        }

        ////////////////////////////////////////////////////////////////////////////////
        // show stuff

        if (options.count("discover")) {
            showLocalDevices();
            exit(0);
        }

        if (options.count("version")) {
            showVersion();
            exit(0);
        }

        if (options.count("help")) {
            help(options);
            die("", 0);
        }

        ////////////////////////////////////////////////////////////////////////////////
        // cert

        if(options.count("create-cert")) {
            if (!options.count("cert-name")) {
                die("Missing cert-name parameter");
            }
            if (certCreate(options["cert-name"].as<std::string>(), options["password"].as<std::string>())) {
                exit(0);
            } else {
                die("Create cert failed");
            }
        }

        if(options.count("certs")) {
            if (certList()) {
                exit(0);
            } else {
                die("list certs failed");
            }
        }

        ////////////////////////////////////////////////////////////////////////////////
        //stream

        nabto_handle_t session;
        nabto_stream_t stream, streamRead;
        nabto_status_t status;
        if (!certOpenSession(session, options)) {
            exit(1);
        }
    
        const char* host(options["deviceid"].as<std::string>().c_str());
        status = nabtoStreamOpen(&stream, session, host);


        if (status == NABTO_OK) {
            std::cout << "nabtoStreamOpen() succeeded, stream = " << stream <<  std::endl;

        } else {
            std::cout << "nabtoStreamOpen() failed with status " << status << ": " << nabtoStatusStr(status) << std::endl;
            nabtoCloseSession(session);
            exit(1);
        }

        std::cout << "Please input test mode: 0(automatic testing), 1(Manual test)" << std::endl;

        size_t mode = 1;
        std::cin >> mode;
/*
        status = nabtoStreamOpen(&streamRead, session, host);
        if (status == NABTO_OK) {
            std::cout << "nabtoStreamOpen() succeeded, stream = " << streamRead <<  std::endl;

        } else {
            std::cout << "nabtoStreamOpen() failed, stream = " << streamRead <<  std::endl;
            std::cout << status << ": " << nabtoStatusStr(status) << std::endl;
            nabtoCloseSession(session);
            exit(1);
        }
*/
   //     std::thread t(readData, streamRead, mode);

        std::thread t(readData, stream, mode);

        if (mode == 0) {
            size_t totalBytes = 0, usedTime;
            size_t i, testCount = 1000;
            time_t start, end;
            std::string testCase = "";
            for (i = 0; i < 100; i++) {
                testCase = testCase + "nabtoSpeed";
            }
            start = time(NULL);
            for (i = 0; i < testCount; i++) {
              //  std::lock_guard<std::mutex> lock(iomutex_);
              //  pthread_mutex_lock(&(streamLock));
                status = nabtoStreamWrite(stream, testCase.c_str(), testCase.size());
             //   pthread_mutex_unlock(&(streamLock));
                if (status != NABTO_OK) {
                    NABTO_LOG_TRACE("Stream wrote failed with i = %d, status = %d", i, status);
                    nabtoStreamClose(stream);
                    break;
                }
                totalBytes = totalBytes + testCase.size();
                NABTO_LOG_TRACE("send bytes:  %d", totalBytes);
                usleep(1000);
            }
            end = time(NULL);
            usedTime = (size_t)(end - start);
            NABTO_LOG_TRACE("sendTimeSt = %ld, sendTimeEd = %ld", start, end);
            NABTO_LOG_TRACE("usedTime = %ds", usedTime);
            NABTO_LOG_TRACE("You have send %d Bytes", totalBytes);
            NABTO_LOG_TRACE("stream send rate is %.2f KBps", 1.0*totalBytes/usedTime/1024);
         //   nabtoStreamClose(stream);
        } else if (mode == 1) {
            NABTO_LOG_TRACE("Stream opened. Type data to send and press <enter> to send. Send the EOF character (%s) to close stream.", eofSequence.c_str());
            size_t sum = 0;
            for (;;) {
                std::string input;
                try {
                    std::cin >> input;
                    if(std::cin.eof()){
                        NABTO_LOG_TRACE("Reached EOF, closing stream");
                        nabtoStreamClose(stream);
                        break;
                    }
                } catch (...) {
                    NABTO_LOG_TRACE("input error");
                    nabtoStreamClose(stream);
                    break;
                }
                NABTO_LOG_TRACE("start sending...");
                status = nabtoStreamWrite(stream, input.c_str(), input.size());
                if (status != NABTO_OK) {
                    NABTO_LOG_TRACE("Stream wrote failed with status = %d", status);
                    nabtoStreamClose(stream);
                    break;
                }
                sum += input.size();
                NABTO_LOG_TRACE("send sum bytes = %d", sum);
            }
        //    NABTO_LOG_TRACE("send sum bytes = %d", sum);
        } else {
            die ("nabto write read mode error", 0);
        }
        t.join();
        nabtoStreamClose(stream);
        nabtoStreamClose(streamRead);
        std::cout << "Stream closed. Cleaning up." << std::endl;

        nabtoCloseSession(session);
        nabtoShutdown();

    }
    catch (const cxxopts::OptionException& e)
    {
        std::cout << "Error parsing options: " << e.what() << std::endl;
        exit(1);
    }

    return 0;
}


