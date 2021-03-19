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
#include "json/json.h"
#include "json/json_helper.hpp"
#include "tunnel_manager/tunnel_manager.hpp"

#include <time.h>
#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include <string>
#include <memory>
#include <chrono>
#include <iomanip>

const std::string eofSequence = "Ctrl+d";

bool init(cxxopts::Options& options) {
    nabto_status_t st;
    if (options.count("home-dir")) {
        st = nabtoStartup(options["home-dir"].as<std::string>().c_str());
    } else {
        st = nabtoStartup(NULL);
    }
    return st == NABTO_OK && nabtoInstallDefaultStaticResources(NULL) == NABTO_OK;
}

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

void die(const std::string& msg, int status=1) {
    std::cout << msg << std::endl;
    nabtoShutdown();
    exit(status);
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

void readData(nabto_stream_t stream)
{
    size_t sum=0;
    time_t t1,t2;
    time(&t1);
    printf("t1 = %ld\n",t1);
    for (;;) {
        size_t actual = 0; /* actual size (in bytes) of response */
        char* response;
        nabto_status_t status = nabtoStreamRead(stream, &response, &actual);
     //   std::cout << "got " << actual << " bytes with status " << status << std::endl;
        if (status == NABTO_OK) {
         //   std::string responseStr(response, actual);
        //    std::cout << "Received stream data: " << responseStr << std::endl;
            sum = sum + actual;
        } else {
            break;
        }
    }
    time(&t2);
    printf("t2 = %ld\n",t2);
    size_t usedTime = (size_t)(t2-t1);
    printf("usedTime = %d\n",usedTime);
    printf("you should receive 140000 Bytes, you actually receive %d Bytes\n", sum);
    printf("Transmission rate is %d Bps\n",sum/usedTime);
}

int main(int argc, char** argv)
{
    try
    {
        cxxopts::Options options(argv[0], "Nabto stream echo client example.");
        options.add_options()
            ("c,create-cert", "Create self signed certificate")
            ("n,cert-name", "Certificate name. ex.: nabto-user", cxxopts::value<std::string>())
            ("a,password", "Password for encrypting private key", cxxopts::value<std::string>()->default_value("not-so-secret"))
            ("d,deviceid", "Device ID to connect to", cxxopts::value<std::string>())
            ("p,productid", "Product ID to use", cxxopts::value<std::string>())
            ("k,serverkey", "Server key of the app", cxxopts::value<std::string>())
            ("h,help", "Shows this help text");
        options.parse(argc, argv);

        if (!init(options)) {
            die("Initialization failed");
        }

        ////////////////////////////////////////////////////////////////////////////////
        // show stuff

        if (options.count("help")) {
            std::cout << options.help({"", "Group"}) << std::endl;
            die("", 0);
        }

        ////////////////////////////////////////////////////////////////////////////////
        // certs

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

        ////////////////////////////////////////////////////////////////////////////////


        nabto_handle_t session;
        nabto_stream_t stream;
        nabto_status_t status;
        if (!certOpenSession(session, options)) {
            exit(1);
        }
    
        const char* host(options["deviceid"].as<std::string>().c_str());
        status = nabtoStreamOpen(&stream, session, host);

        
        std::thread t(readData, stream);

        if (status == NABTO_OK) {
            //std::lock_guard<std::mutex> lock(iomutex_);
            std::cout << "nabtoStreamOpen() succeeded, stream = " << stream <<  std::endl;

        } else {
            //std::lock_guard<std::mutex> lock(iomutex_);
            std::cout << "nabtoStreamOpen() failed with status " << status << ": " << nabtoStatusStr(status) << std::endl;
           nabtoCloseSession(session);
            exit(1);
        }

        std::cout << "Stream opened. Type data to send and press <enter> to send. Send the EOF character (" << eofSequence << ") to close stream." << std::endl;
/*
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
        }
*/
        int i=0;
        std::string testCase="nabtoSpeedTest";
        for (; i<100000; i++) {
            nabtoStreamWrite(stream, testCase.c_str(), testCase.size());
        }
        nabtoStreamClose(stream);

        //std::lock_guard<std::mutex> lock(iomutex_);

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
/***********************
 * stream 客户端验证
        
        std::cout << "Stream opened. Type stream_echo to verificate!" << std::endl;
        std::string stream_echo_verification;
        std::cin >> stream_echo_verification;
        status = nabtoStreamWrite(stream, stream_echo_verification.c_str(), stream_echo_verification.size());
        if (status != NABTO_OK) {
            die("unabto stream verificated failed");
        }
        */