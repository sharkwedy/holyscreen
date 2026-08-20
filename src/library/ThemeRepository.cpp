#include "library/ThemeRepository.h"
#include <QDir>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QUuid>

namespace churchpresenter {
ThemeRepository::ThemeRepository(QString path):m_databasePath(std::move(path)){}
ThemeRepository::~ThemeRepository(){ if(m_connectionName.isEmpty())return; {auto db=QSqlDatabase::database(m_connectionName,false);if(db.isValid())db.close();} QSqlDatabase::removeDatabase(m_connectionName); }
bool ThemeRepository::open(){
 if(!m_connectionName.isEmpty()&&QSqlDatabase::database(m_connectionName,false).isOpen())return true;
 if(!QDir().mkpath(QFileInfo(m_databasePath).absolutePath()))return false;
 m_connectionName="holyscreen-themes-"+QUuid::createUuid().toString(QUuid::WithoutBraces);
 auto db=QSqlDatabase::addDatabase("QSQLITE",m_connectionName);db.setDatabaseName(m_databasePath);if(!db.open())return false;
 QSqlQuery q(db);return q.exec("CREATE TABLE IF NOT EXISTS themes(id TEXT PRIMARY KEY,name TEXT NOT NULL,background_type INTEGER NOT NULL,background_color TEXT NOT NULL,background_image TEXT NOT NULL,font_family TEXT NOT NULL,font_size INTEGER NOT NULL,minimum_font_size INTEGER NOT NULL,font_weight INTEGER NOT NULL,text_color TEXT NOT NULL,horizontal_alignment TEXT NOT NULL,vertical_alignment TEXT NOT NULL,line_spacing INTEGER NOT NULL,margin INTEGER NOT NULL,outline INTEGER NOT NULL,outline_color TEXT NOT NULL,shadow INTEGER NOT NULL,shadow_color TEXT NOT NULL,transition_name TEXT NOT NULL)");
}
static Theme readTheme(QSqlQuery&q){return Theme{.id=q.value(0).toString(),.name=q.value(1).toString(),.backgroundType=static_cast<BackgroundType>(q.value(2).toInt()),.backgroundColor=q.value(3).toString(),.backgroundImage=q.value(4).toString(),.fontFamily=q.value(5).toString(),.fontSize=q.value(6).toInt(),.minimumFontSize=q.value(7).toInt(),.fontWeight=q.value(8).toInt(),.textColor=q.value(9).toString(),.horizontalAlignment=q.value(10).toString(),.verticalAlignment=q.value(11).toString(),.lineSpacing=q.value(12).toInt(),.margin=q.value(13).toInt(),.outline=q.value(14).toBool(),.outlineColor=q.value(15).toString(),.shadow=q.value(16).toBool(),.shadowColor=q.value(17).toString(),.transition=q.value(18).toString()};}
QVector<Theme> ThemeRepository::themes()const{QVector<Theme>r;QSqlQuery q(QSqlDatabase::database(m_connectionName,false));if(q.exec("SELECT * FROM themes ORDER BY name COLLATE NOCASE,id"))while(q.next())r.append(readTheme(q));return r;}
Theme ThemeRepository::theme(const QString&id)const{QSqlQuery q(QSqlDatabase::database(m_connectionName,false));q.prepare("SELECT * FROM themes WHERE id=:id");q.bindValue(":id",id);return q.exec()&&q.next()?readTheme(q):Theme{};}
QString ThemeRepository::save(Theme t){if(m_connectionName.isEmpty()&&!open())return{};if(t.id.isEmpty())t.id=QUuid::createUuid().toString(QUuid::WithoutBraces);if(t.name.trimmed().isEmpty())t.name="Tema";
 auto nonNull=[](QString&s){if(s.isNull())s="";};nonNull(t.backgroundColor);nonNull(t.backgroundImage);nonNull(t.fontFamily);nonNull(t.textColor);nonNull(t.horizontalAlignment);nonNull(t.verticalAlignment);nonNull(t.outlineColor);nonNull(t.shadowColor);nonNull(t.transition);
 QSqlQuery q(QSqlDatabase::database(m_connectionName,false));q.prepare("INSERT INTO themes VALUES(:id,:name,:bt,:bc,:bi,:ff,:fs,:min,:fw,:tc,:ha,:va,:ls,:margin,:outline,:oc,:shadow,:sc,:tr) ON CONFLICT(id) DO UPDATE SET name=excluded.name,background_type=excluded.background_type,background_color=excluded.background_color,background_image=excluded.background_image,font_family=excluded.font_family,font_size=excluded.font_size,minimum_font_size=excluded.minimum_font_size,font_weight=excluded.font_weight,text_color=excluded.text_color,horizontal_alignment=excluded.horizontal_alignment,vertical_alignment=excluded.vertical_alignment,line_spacing=excluded.line_spacing,margin=excluded.margin,outline=excluded.outline,outline_color=excluded.outline_color,shadow=excluded.shadow,shadow_color=excluded.shadow_color,transition_name=excluded.transition_name");
 q.bindValue(":id",t.id);q.bindValue(":name",t.name);q.bindValue(":bt",static_cast<int>(t.backgroundType));q.bindValue(":bc",t.backgroundColor);q.bindValue(":bi",t.backgroundImage);q.bindValue(":ff",t.fontFamily);q.bindValue(":fs",t.fontSize);q.bindValue(":min",t.minimumFontSize);q.bindValue(":fw",t.fontWeight);q.bindValue(":tc",t.textColor);q.bindValue(":ha",t.horizontalAlignment);q.bindValue(":va",t.verticalAlignment);q.bindValue(":ls",t.lineSpacing);q.bindValue(":margin",t.margin);q.bindValue(":outline",t.outline);q.bindValue(":oc",t.outlineColor);q.bindValue(":shadow",t.shadow);q.bindValue(":sc",t.shadowColor);q.bindValue(":tr",t.transition);return q.exec()?t.id:QString{};}
bool ThemeRepository::remove(const QString&id){QSqlQuery q(QSqlDatabase::database(m_connectionName,false));q.prepare("DELETE FROM themes WHERE id=:id");q.bindValue(":id",id);return q.exec()&&q.numRowsAffected()==1;}
}
