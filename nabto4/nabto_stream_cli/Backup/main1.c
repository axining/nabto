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
#include "nabto_client_api.h"
#include "cxxopts.hpp"

#include <time.h>
#include <iostream>
#include <fstream>
#include <thread>
#include <string>
#include <memory>
#include <chrono>
#include <iomanip>

const std::string eofSequence = "Ctrl+d";


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

void readData(nabto_stream_t stream, size_t mode)
{
    if (mode) {
        for (;;) {
            size_t actual = 0; /* actual size (in bytes) of response */
            char* response;
            nabto_status_t status = nabtoStreamRead(stream, &response, &actual);
            std::cout << "got " << actual << " bytes with status " << status << std::endl;
            if (status == NABTO_OK) {
                std::string responseStr(response, actual);
                std::cout << "Received stream data: " << responseStr << std::endl;
            } else {
                break;
            }
        }
    } else {
        size_t totalBytes=0, usedTime;
        time_t start, end;
        bool revTag = false;
        size_t actual = 0; /* actual size (in bytes) of response */
        char* response;
        for (;;) {
            nabto_status_t status = nabtoStreamRead(stream, &response, &actual);
            if (status == NABTO_OK) {
                totalBytes = totalBytes + actual;
                if (revTag == false) {
                    start = time(NULL);
                    revTag = true;
                }
            } else {
                break;
            }
        }
        end = time(NULL);
        usedTime = (size_t)(end-start);
        printf ("revTimeSt = %ld\nrevTimeEd = %ld\n",start, end);
        printf ("usedTime = %ds\n", usedTime);
        printf ("You have received %d Bytes\n", totalBytes);
        printf ("stream received rate is %.2f KBps\n", 1.0*totalBytes/usedTime/1024);
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
        nabto_stream_t stream;
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

        size_t mode = 0;
        std::cout << "Please input test mode: 0(automatic testing), 1(Manual test)" << std::endl;
        std::cin >> mode;

        std::thread t(readData, stream, mode);

        if (mode) {
            std::cout << "Stream opened. Type data to send and press <enter> to send. Send the EOF character (" << eofSequence << ") to close stream." << std::endl;
            for (;;) {
                std::string input;
                try {
                    std::cin >> input;
                    if(std::cin.eof()){
                        std::cout << "Reached EOF, closing stream" << std::endl;
                        nabtoStreamClose(stream);
                        break;
                    }
                } catch (...) {
                    nabtoStreamClose(stream);
                    break;
                }
                nabtoStreamWrite(stream, input.c_str(), input.size());
                if (status != NABTO_OK) {
                    std::cout << "Stream wrote failed!\n " <<  std::endl;
                    nabtoStreamClose(stream);
                    break;
                }
            }
        } else {
            time_t writeStartTime, writeEndTime, writeUsedTime;
            size_t i, testCount = 10;
            size_t writeTotalBytes=0;
            std::string testCase;
            for (i=0; i<10000; i++) {
                testCase = testCase + "nabtoSpeedTest";
            }
            writeStartTime = time(NULL);
            for (i=0; i<testCount; i++) {
                nabtoStreamWrite(stream, testCase.c_str(), testCase.size());
                if (status != NABTO_OK) {
                    std::cout << "Stream wrote failed!\n " <<  std::endl;
                    nabtoStreamClose(stream);
                }
                writeTotalBytes = writeTotalBytes + testCase.size();
            }
            writeEndTime = time(NULL);
            nabtoStreamClose(stream);
            writeUsedTime = (size_t)(writeEndTime-writeStartTime);
            printf ("sendTimeSt = %ld\nsendTimeEd = %ld\n",writeStartTime, writeEndTime);
            printf ("writeUsedTime = %ds\n", writeUsedTime);
            printf ("You have send %d Bytes\n", writeTotalBytes);
            printf ("stream send rate is %.2f KBps\n", 1.0*writeTotalBytes/writeUsedTime/1024);
            
        }

        t.join();
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

