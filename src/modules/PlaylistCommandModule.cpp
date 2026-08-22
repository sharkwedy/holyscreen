#include "modules/PlaylistCommandModule.h"

#include <QDateTime>
#include <QUuid>

namespace churchpresenter {

PlaylistCommandModule::PlaylistCommandModule(CommandBus &commands, EventBus &events,
                                             Actions actions, UndoManager *undoManager,
                                             QObject *parent)
    : QObject(parent), m_commands(commands), m_events(events), m_actions(std::move(actions)),
      m_undoManager(undoManager)
{
    m_commands.registerHandler(QStringLiteral("media.playlist.move"),
                               [this](const Command &command) {
        const auto id=command.payload.value(QStringLiteral("id")).toString().trimmed();
        bool valid=false;const auto index=command.payload.value(QStringLiteral("index")).toInt(&valid);
        if(id.isEmpty()||!valid||index<0)return CommandResult{.accepted=false,.errorCode=QStringLiteral("invalid_payload"),.message=QStringLiteral("id e index são obrigatórios.")};
        return change(command,QStringLiteral("Mover item da playlist"),[this,id,index]{return m_actions.move&&m_actions.move(id,index);});
    });
    m_commands.registerHandler(QStringLiteral("media.playlist.remove"),
                               [this](const Command &command) {
        const auto id=command.payload.value(QStringLiteral("id")).toString().trimmed();
        if(id.isEmpty())return CommandResult{.accepted=false,.errorCode=QStringLiteral("invalid_payload"),.message=QStringLiteral("id é obrigatório.")};
        return change(command,QStringLiteral("Remover item da playlist"),[this,id]{return m_actions.remove&&m_actions.remove(id);});
    });
    m_commands.registerHandler(QStringLiteral("media.playlist.clear"),
                               [this](const Command &command) {
        return change(command,QStringLiteral("Limpar playlist"),[this]{return m_actions.clear&&m_actions.clear();});
    });
}

CommandResult PlaylistCommandModule::dispatch(const QString&type,const QVariantMap&payload,const QString&source)
{return m_commands.dispatch(Command{.id=QUuid::createUuid().toString(QUuid::WithoutBraces),.type=type,.payload=payload,.source=source,.issuedAt=QDateTime::currentDateTimeUtc()});}
CommandResult PlaylistCommandModule::requestMove(const QString&id,int index,const QString&source){return dispatch(QStringLiteral("media.playlist.move"),{{QStringLiteral("id"),id},{QStringLiteral("index"),index}},source);}
CommandResult PlaylistCommandModule::requestRemove(const QString&id,const QString&source){return dispatch(QStringLiteral("media.playlist.remove"),{{QStringLiteral("id"),id}},source);}
CommandResult PlaylistCommandModule::requestClear(const QString&source){return dispatch(QStringLiteral("media.playlist.clear"),{},source);}

CommandResult PlaylistCommandModule::change(const Command&command,const QString&label,const std::function<bool()>&operation)
{
    const auto before=m_actions.snapshot?m_actions.snapshot():QVariantList{};
    if(!operation||!operation())return {.accepted=false,.errorCode=QStringLiteral("operation_failed"),.message=QStringLiteral("A playlist não pôde ser atualizada.")};
    const auto after=m_actions.snapshot?m_actions.snapshot():QVariantList{};
    publish(command.type,command.id);
    if(m_undoManager&&before!=after)m_undoManager->record(label,
        [this,before]{return restore(before,QUuid::createUuid().toString(QUuid::WithoutBraces));},
        [this,after]{return restore(after,QUuid::createUuid().toString(QUuid::WithoutBraces));});
    return {.accepted=true,.message=QStringLiteral("Playlist atualizada.")};
}
bool PlaylistCommandModule::restore(const QVariantList&snapshot,const QString&correlationId){return m_actions.restore&&m_actions.restore(snapshot)&&publish(QStringLiteral("restore"),correlationId);}
bool PlaylistCommandModule::publish(const QString&action,const QString&correlationId){const auto items=m_actions.snapshot?m_actions.snapshot():QVariantList{};return m_events.publish(DomainEvent{.type=QStringLiteral("media.playlist.changed"),.payload={{QStringLiteral("action"),action},{QStringLiteral("items"),items},{QStringLiteral("count"),items.size()}},.occurredAt=QDateTime::currentDateTimeUtc(),.correlationId=correlationId});}

} // namespace churchpresenter
