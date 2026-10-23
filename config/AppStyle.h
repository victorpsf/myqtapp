#pragma once

#include <QString>

namespace AppStyle {

inline QString styleSheet()
{
    return QStringLiteral(R"(
        QMainWindow, QStackedWidget > QWidget { background: #f3f4f6; }

        QFrame#FormCard {
            background: #ffffff;
            border: 1px solid #e5e7eb;
            border-radius: 12px;
        }

        QLabel[variant="title"]    { font-size: 22px; font-weight: 600; color: #111827; }
        QLabel[variant="section"]  { font-size: 15px; font-weight: 600; color: #111827; }
        QLabel[variant="subtitle"] { font-size: 14px; color: #4b5563; }
        QLabel[variant="body"]     { font-size: 13px; color: #111827; }
        QLabel[variant="caption"]  { font-size: 12px; color: #6b7280; }

        QLineEdit {
            padding: 8px 10px;
            border: 1px solid #d1d5db;
            border-radius: 6px;
            font-size: 14px;
            background: #ffffff;
        }
        QLineEdit:focus { border-color: #2563eb; }
        QComboBox {
            padding: 6px 10px; border: 1px solid #d1d5db; border-radius: 6px;
            background: #ffffff; font-size: 14px; color: #111827;
        }
        QComboBox:focus { border-color: #2563eb; }
        QComboBox:disabled, QLineEdit:disabled, QPlainTextEdit:disabled { color: #9ca3af; background: #f9fafb; }
        QPlainTextEdit {
            padding: 6px; border: 1px solid #d1d5db; border-radius: 6px;
            background: #ffffff; font-size: 13px; color: #111827;
        }
        QPlainTextEdit:focus { border-color: #2563eb; }
        QLineEdit:read-only, QPlainTextEdit[readOnly="true"] { background: #f9fafb; }
        QRadioButton { font-size: 13px; color: #111827; spacing: 6px; }

        /* ---------- Páginas de conteúdo ---------- */
        QScrollArea#ContentPage, QScrollArea#ContentPage > QWidget,
        QWidget#ContentPageBody { background: #f3f4f6; border: none; }
        QFrame#SectionCard { background: #ffffff; border: 1px solid #e5e7eb; border-radius: 10px; }

        /* ---------- Menu superior ---------- */
        QFrame#TopBar { background: #ffffff; border: none; border-bottom: 1px solid #e5e7eb; }
        QLabel#TopBarTitle { font-size: 16px; font-weight: 600; color: #111827; }
        QToolButton#TopBarMenuButton {
            font-size: 20px; color: #111827; border: none; border-radius: 6px; padding: 4px 10px;
        }
        QToolButton#TopBarUserButton {
            font-size: 14px; color: #111827; border: none; border-radius: 20px; padding: 4px 12px 4px 4px;
        }
        QToolButton#TopBarLogoutButton {
            font-size: 13px; color: #4b5563; border: 1px solid #d1d5db; border-radius: 6px; padding: 6px 12px;
        }
        QToolButton#TopBarMenuButton:hover, QToolButton#TopBarUserButton:hover,
        QToolButton#TopBarLogoutButton:hover { background: #f3f4f6; }

        /* ---------- Menu lateral ---------- */
        QFrame#SideMenu { background: #111827; border: none; }
        QScrollArea#SideMenuScroll, QWidget#SideMenuContent, QFrame#SideMenu QWidget { background: transparent; }
        QPushButton#SideMenuGroup { border: none; border-radius: 0; padding: 0; background: transparent; }
        QPushButton#SideMenuGroup:hover { background: #1f2937; }
        QLabel#SideMenuIcon  { font-size: 18px; color: #e5e7eb; }
        QLabel#SideMenuText  { font-size: 14px; font-weight: 600; color: #e5e7eb; }
        QLabel#SideMenuArrow { font-size: 12px; color: #9ca3af; }
        QPushButton#SideMenuItem {
            border: none; border-radius: 6px; margin: 0 8px; padding: 8px 12px 8px 44px;
            text-align: left; font-size: 13px; color: #9ca3af; background: transparent;
        }
        QPushButton#SideMenuItem:hover { background: #1f2937; color: #f9fafb; }
        QPushButton#SideMenuItem:checked { background: #2563eb; color: #ffffff; }

        /* ---------- Perfil ---------- */
        QDialog#ProfileWindow, QDialog#ProfileWindow QStackedWidget > QWidget { background: #ffffff; }
        QProgressBar { border: none; border-radius: 3px; background: #e5e7eb; }
        QProgressBar::chunk { border-radius: 3px; background: #2563eb; }

        QLineEdit[code="true"] { font-size: 24px; font-weight: 600; padding: 8px; }

        QPushButton {
            padding: 8px 16px;
            border-radius: 6px;
            font-size: 14px;
            border: 1px solid #d1d5db;
            background: #ffffff;
            color: #111827;
        }
        QPushButton:hover { background: #f9fafb; }
        QPushButton:disabled { color: #9ca3af; }

        QPushButton[primary="true"] {
            background: #2563eb;
            border-color: #2563eb;
            color: #ffffff;
        }
        QPushButton[primary="true"]:hover { background: #1d4ed8; }
        QPushButton[primary="true"]:disabled { background: #93c5fd; border-color: #93c5fd; }
    )");
}

} // namespace AppStyle
