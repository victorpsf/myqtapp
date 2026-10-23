cmake_minimum_required(VERSION 3.16)

project(myqtapp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Tipo de build" FORCE)
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Concurrent)
find_package(OpenSSL 1.1.1 REQUIRED)

set(SOURCES
    main.cpp
    MainWindow.h MainWindow.cpp

    config/Env.h config/Env.cpp
    config/AppConfig.h config/AppConfig.cpp
    config/AppStyle.h

    models/entity/User.h
    models/entity/UserProfile.h
    models/requests/AuthenticationRequest.h models/requests/AuthenticationRequest.cpp
    models/requests/HandshakeRequest.h
    models/responses/IntegrationResponse.h
    models/responses/IdentifyResponse.h
    models/responses/LoginResponse.h
    models/responses/TwoFactorResponse.h
    models/responses/ResendCodeResponse.h
    models/responses/HandshakeResponse.h
    models/responses/EncryptedPayload.h

    security/OpenSsl.h security/OpenSsl.cpp
    security/Random.h security/Random.cpp
    security/Hash.h security/Hash.cpp
    security/KeyStore.h security/KeyStore.cpp
    security/SecuritySession.h security/SecuritySession.cpp
    security/ContentPackage.h security/ContentPackage.cpp
    security/symmetric/Aes256Cbc.h security/symmetric/Aes256Cbc.cpp
    security/symmetric/PasswordEncryption.h security/symmetric/PasswordEncryption.cpp
    security/asymmetric/Rsa.h security/asymmetric/Rsa.cpp
    security/asymmetric/SecureEnvelope.h security/asymmetric/SecureEnvelope.cpp
    security/asymmetric/AsymmetricKeyGenerator.h security/asymmetric/AsymmetricKeyGenerator.cpp
    security/asymmetric/PublicKeyEncryption.h security/asymmetric/PublicKeyEncryption.cpp

    integration/HttpClient.h integration/HttpClient.cpp
    integration/auth/AuthIntegration.h integration/auth/AuthIntegration.cpp
    integration/personal/PersonalIntegration.h integration/personal/PersonalIntegration.cpp

    components/labels/TextLabel.h components/labels/TextLabel.cpp
    components/labels/AvatarLabel.h components/labels/AvatarLabel.cpp
    components/labels/ProgressStatus.h components/labels/ProgressStatus.cpp
    components/menus/TopBar.h components/menus/TopBar.cpp
    components/menus/SideMenu.h components/menus/SideMenu.cpp
    components/fields/InputField.h components/fields/InputField.cpp
    components/fields/SelectField.h components/fields/SelectField.cpp
    components/fields/TextAreaField.h components/fields/TextAreaField.cpp
    components/fields/FilePickerField.h components/fields/FilePickerField.cpp
    components/fields/ContentSourceField.h components/fields/ContentSourceField.cpp
    components/forms/FormCard.h components/forms/FormCard.cpp
    components/forms/SectionCard.h components/forms/SectionCard.cpp
    components/forms/ContentPage.h components/forms/ContentPage.cpp
    components/popups/Popup.h components/popups/Popup.cpp

    screens/LoginIdentifierScreen.h screens/LoginIdentifierScreen.cpp
    screens/LoginPasswordScreen.h screens/LoginPasswordScreen.cpp
    screens/TwoFactorScreen.h screens/TwoFactorScreen.cpp
    screens/HomeScreen.h screens/HomeScreen.cpp
    screens/ProfileWindow.h screens/ProfileWindow.cpp
    screens/keys/KeyToolsCommon.h screens/keys/KeyToolsCommon.cpp
    screens/keys/AsymmetricKeyScreen.h screens/keys/AsymmetricKeyScreen.cpp
    screens/keys/SymmetricEncryptionScreen.h screens/keys/SymmetricEncryptionScreen.cpp
    screens/keys/HashScreen.h screens/keys/HashScreen.cpp
    screens/keys/EncryptedResultCard.h screens/keys/EncryptedResultCard.cpp
    screens/keys/DecryptedResultCard.h screens/keys/DecryptedResultCard.cpp
    screens/keys/AsymmetricEncryptionScreen.h screens/keys/AsymmetricEncryptionScreen.cpp
    screens/keys/AsymmetricDecryptionScreen.h screens/keys/AsymmetricDecryptionScreen.cpp
    screens/keys/SymmetricDecryptionScreen.h screens/keys/SymmetricDecryptionScreen.cpp
)

# WIN32: sem console no Windows | MACOSX_BUNDLE: gera myqtapp.app no Mac (ignorados no Linux)
add_executable(myqtapp WIN32 MACOSX_BUNDLE ${SOURCES})

target_include_directories(myqtapp PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(myqtapp PRIVATE Qt6::Widgets Qt6::Network Qt6::Concurrent OpenSSL::Crypto)

set(APP_DISPLAY_NAME "MyQtApp")
set(APP_BUNDLE_ID "local.myqtapp" CACHE STRING "Identificador do app no macOS")
set(PACKAGE_MAINTAINER "MyQtApp <maintainer@example.com>" CACHE STRING
    "Mantenedor exibido no pacote .deb")

set_target_properties(myqtapp PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME ${APP_DISPLAY_NAME}
    MACOSX_BUNDLE_GUI_IDENTIFIER ${APP_BUNDLE_ID}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
)

# Copia o .env para junto do executável (o Env procura primeiro lá)
if(EXISTS ${CMAKE_SOURCE_DIR}/.env)
    configure_file(${CMAKE_SOURCE_DIR}/.env ${CMAKE_BINARY_DIR}/.env COPYONLY)
endif()

# .env embutido nos pacotes (padrão: .env, ou .env.example se não existir)
if(EXISTS ${CMAKE_SOURCE_DIR}/.env)
    set(_default_env_file ${CMAKE_SOURCE_DIR}/.env)
else()
    set(_default_env_file ${CMAKE_SOURCE_DIR}/.env.example)
endif()
set(APP_ENV_FILE ${_default_env_file} CACHE FILEPATH ".env embutido nos pacotes")

# ---------------------------------------------------------------- instalação
# `cmake --install` / CPack (usados pelo scripts/package.sh)
include(GNUInstallDirs)

if(APPLE)
    install(TARGETS myqtapp BUNDLE DESTINATION .)
    install(FILES ${APP_ENV_FILE} DESTINATION myqtapp.app/Contents/MacOS RENAME .env)
elseif(WIN32)
    install(TARGETS myqtapp RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(FILES ${APP_ENV_FILE} DESTINATION ${CMAKE_INSTALL_BINDIR} RENAME .env)

    # O windeployqt não copia o OpenSSL: procura as DLLs na instalação do OpenSSL
    get_filename_component(_openssl_root "${OPENSSL_INCLUDE_DIR}/.." ABSOLUTE)
    file(GLOB OPENSSL_RUNTIME_DLLS "${_openssl_root}/bin/libcrypto*.dll"
                                   "${_openssl_root}/libcrypto*.dll")
    if(OPENSSL_RUNTIME_DLLS)
        install(FILES ${OPENSSL_RUNTIME_DLLS} DESTINATION ${CMAKE_INSTALL_BINDIR})
    else()
        message(WARNING "DLL do OpenSSL (libcrypto*.dll) não encontrada em ${_openssl_root}")
    endif()
else()
    install(TARGETS myqtapp RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(FILES ${APP_ENV_FILE} DESTINATION ${CMAKE_INSTALL_DATADIR}/myqtapp RENAME .env)
    install(FILES packaging/myqtapp.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES packaging/myqtapp.svg
            DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
endif()

# Windows/Mac: copia Qt e plugins para dentro do pacote (windeployqt / macdeployqt).
# No Linux o .deb usa o Qt do sistema e o AppImage usa o linuxdeploy.
if(WIN32 OR APPLE)
    if(Qt6_VERSION VERSION_LESS 6.5)
        message(WARNING "Empacotamento no Windows/Mac requer Qt 6.5 ou mais novo")
    else()
        qt_generate_deploy_app_script(
            TARGET myqtapp
            OUTPUT_SCRIPT deploy_script
            NO_UNSUPPORTED_PLATFORM_ERROR
        )
        install(SCRIPT ${deploy_script})
    endif()
endif()

# ---------------------------------------------------------------- pacotes (CPack)
set(CPACK_PACKAGE_NAME myqtapp)
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VENDOR ${APP_DISPLAY_NAME})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Controle financeiro e ferramentas de segurança")
set(CPACK_PACKAGE_INSTALL_DIRECTORY ${APP_DISPLAY_NAME})
set(CPACK_PACKAGE_EXECUTABLES myqtapp ${APP_DISPLAY_NAME})
set(CPACK_STRIP_FILES ON)

# .deb (Linux): dependências do Qt/OpenSSL calculadas pelo dpkg-shlibdeps
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER ${PACKAGE_MAINTAINER})
set(CPACK_DEBIAN_PACKAGE_SECTION utils)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

# .exe (Windows): instalador NSIS com atalho no menu Iniciar e desinstalador
set(CPACK_NSIS_DISPLAY_NAME ${APP_DISPLAY_NAME})
set(CPACK_NSIS_PACKAGE_NAME ${APP_DISPLAY_NAME})
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_NSIS_INSTALLED_ICON_NAME bin/myqtapp.exe)

# .dmg (Mac): arrastar o app para Aplicativos
set(CPACK_DMG_VOLUME_NAME ${APP_DISPLAY_NAME})

if(APPLE)
    set(CPACK_GENERATOR DragNDrop)
elseif(WIN32)
    set(CPACK_GENERATOR NSIS ZIP)
else()
    set(CPACK_GENERATOR DEB)
endif()

include(CPack)

# `make run` / `cmake --build build --target run`
add_custom_target(run
    COMMAND $<TARGET_FILE:myqtapp>
    DEPENDS myqtapp
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    USES_TERMINAL
)
