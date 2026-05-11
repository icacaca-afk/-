#ifndef COMMANDITEM_H
#define COMMANDITEM_H

#pragma once
#include <QString>
#include <QList>
#include <QIcon>

struct CommandItem {
    enum Type { Command, Menu, AIAction };

    Type type;
    QString name;
    QIcon icon;
    QString cmd;       // 仅command使用
    QList<CommandItem> items; // 仅menu使用
    QString prompt;        // 仅ai-action使用：发送给AI的提示词
    QString systemPrompt;  // 仅ai-action使用：系统提示词（可选）

    // 正确定义的构造函数
    CommandItem(Type t = Command,
                const QString& n = "",
                const QIcon& i = QIcon(),
                const QString& c = "",
                const QList<CommandItem>& subItems = {},
                const QString& p = "",
                const QString& sp = "")
        : type(t), name(n), icon(i), cmd(c), items(subItems), prompt(p), systemPrompt(sp) {}
};

#endif // COMMANDITEM_H
