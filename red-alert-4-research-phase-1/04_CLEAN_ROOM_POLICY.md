# 04. Clean-room policy для коммерческого проекта

> Это инженерная политика снижения риска, а не юридическое заключение. Перед коммерческим релизом нужна проверка юристом по интеллектуальной собственности.

## 4.1. Запрещённые действия

В production-репозиторий нельзя помещать:

- EA C++ source files или их почти буквальный перевод;
- RA3 XML как runtime data;
- EA XSD, shaders, W3X, TGA, 3ds Max files;
- оригинальные названия юнитов, фракций, abilities, персонажей и миссий;
- оригинальные числовые таблицы баланса;
- EA-аудио, музыку, voice lines, UI sprites, logos;
- generated assets, построенные как производная копия конкретного EA-ресурса;
- слово `Red Alert` в коммерческом названии без лицензии.

## 4.2. Разрешённый результат исследования

Можно сохранять самостоятельно написанные документы, содержащие:

- общие архитектурные границы;
- перечни требований к подсистемам;
- абстрактные диаграммы потоков;
- независимые интерфейсы;
- тестовые инварианты;
- собственную таксономию игровых ролей;
- сравнительные таблицы без копирования значимых массивов значений;
- ссылки и provenance metadata.

## 4.3. Разделение ролей

Рекомендуемый процесс:

```text
Researcher
  читает EA-материалы
  → пишет нейтральную спецификацию поведения и ограничений

Clean-room implementer
  получает только спецификацию
  → пишет Unreal-код самостоятельно

Reviewer
  проверяет отсутствие literal similarity, EA identifiers и запрещённых файлов
```

На маленькой команде физическое разделение людей может быть невозможно. Тогда применяются отдельные ветки, контексты и обязательный review журнала происхождения.

## 4.4. Структура каталогов

```text
Research/RA3_SAGE_Study/          только самостоятельные заметки
ExternalResearch/                 gitignored, локальные копии источников
Source/                           только оригинальный код проекта
Content/                          только оригинальные/лицензированные assets
Build/Compliance/                 scanners и allow/deny lists
```

`ExternalResearch/` не должен попадать в Git, CI artifacts, packaged builds или резервные публичные архивы.

## 4.5. Provenance для каждого production asset

Минимальные поля:

```text
asset_id
path
creator
creation_date
source_type
source_uri_or_contract
license
commercial_use_allowed
contains_third_party_ip
similarity_review_status
reviewer
notes
```

## 4.6. Автоматические проверки CI

Добавить job `compliance-scan`:

- запрещённые расширения и каталоги;
- known EA filenames;
- C&C faction/unit/person names;
- EA copyright headers;
- подозрительные XML namespaces и path prefixes;
- shader signatures/filenames;
- asset manifest без provenance;
- binary files без allowlist.

Результат — блокирующий, а не advisory.

## 4.7. Правило для AI-кодогенерации

Промты для кодовых агентов должны явно говорить:

- не копировать и не переводить EA-код;
- не генерировать API по конкретным EA class names;
- работать по независимой спецификации RA4;
- указывать происхождение любых внешних фрагментов;
- помечать сомнительные места для legal/compliance review.
