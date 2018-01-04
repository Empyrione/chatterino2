#include "singletons/commandmanager.hpp"

#include <QDebug>
#include <QFile>
#include <QRegularExpression>

namespace chatterino {
namespace singletons {
CommandManager &CommandManager::getInstance()
{
    static CommandManager instance;
    return instance;
}

void CommandManager::loadCommands()
{
    //    QFile TextFile("");
    //    QStringList SL;

    //    while (!TextFile.atEnd())
    //        SL.append(TextFile.readLine());
}

void CommandManager::saveCommands()
{
}

void CommandManager::setCommands(const QStringList &_commands)
{
    std::lock_guard<std::mutex> lock(this->mutex);

    this->commands.clear();

    for (const QString &commandRef : _commands) {
        QString command = commandRef;

        if (command.size() == 0) {
            continue;
        }

        if (command.at(0) != '/') {
            command = QString("/") + command;
        }

        QString commandName = command.mid(0, command.indexOf(' '));

        if (this->commands.find(commandName) == this->commands.end()) {
            this->commands.insert(commandName, Command(command));
        }
    }

    this->commandsStringList = _commands;
    this->commandsStringList.detach();
}

QStringList CommandManager::getCommands()
{
    return this->commandsStringList;
}

QString CommandManager::execCommand(QString text)
{
    Command command;
    QStringList words = text.split(' ', QString::SkipEmptyParts);

    {
        std::lock_guard<std::mutex> lock(this->mutex);

        if (words.length() == 0) {
            return text;
        }

        QString commandName = words[0];

        auto it = this->commands.find(commandName);

        if (it == this->commands.end()) {
            return text;
        }

        command = it.value();
    }

    QString result;

    static QRegularExpression parseCommand("(^|[^{])({{)*{(\\d+\\+?)}");

    int lastCaptureEnd = 0;

    auto globalMatch = parseCommand.globalMatch(command.text);
    int matchOffset = 0;

    while (true) {
        QRegularExpressionMatch match = parseCommand.match(command.text, matchOffset);

        if (!match.hasMatch()) {
            break;
        }

        result += command.text.mid(lastCaptureEnd, match.capturedStart() - lastCaptureEnd + 1);

        lastCaptureEnd = match.capturedEnd();
        matchOffset = lastCaptureEnd - 1;

        QString wordIndexMatch = match.captured(3);

        bool plus = wordIndexMatch.at(wordIndexMatch.size() - 1) == '+';
        wordIndexMatch = wordIndexMatch.replace("+", "");

        bool ok;
        int wordIndex = wordIndexMatch.replace("=", "").toInt(&ok);
        if (!ok || wordIndex == 0) {
            result += "{" + match.captured(3) + "}";
            continue;
        }

        if (words.length() <= wordIndex) {
            continue;
        }

        if (plus) {
            bool first = true;
            for (int i = wordIndex; i < words.length(); i++) {
                if (!first) {
                    result += " ";
                }
                result += words[i];
                first = false;
            }
        } else {
            result += words[wordIndex];
        }
    }

    result += command.text.mid(lastCaptureEnd);

    if (result.size() > 0 && result.at(0) == '{') {
        result = result.mid(1);
    }

    return result.replace("{{", "{");
}

CommandManager::Command::Command(QString _text)
{
    int index = _text.indexOf(' ');

    if (index == -1) {
        this->name == _text;
        return;
    }

    this->name = _text.mid(0, index);
    this->text = _text.mid(index + 1);
}
}
}
