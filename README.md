message-broker
==============

1. Basic: Der Message Broker verwaltet als zentrale Einheit eine Menge von Topics. Topics werden mit der ersten Nachricht oder dem ersten Subscriber dynamisch erstellt und durch eine Zeichenkette identifiziert. Ein Topic ist ein n:n Kanal mit 0-n Publisher und 0-n Subscriber.
2. Publisher: Der Message Broker akzeptiert Verbindungen von Clients als Publisher. Publisher senden Nachrichten mit beliebigen Inhalt an einen bestimmten Topic.
3. Subscriber: Der Message Broker akzeptiert Verbindungen von Clients als Subscriber. Subscriber geben beim Verbindungsaufbau 1-n Topics an, für welche sie Nachrichten erhalten wollen. Nachdem der Subscriber eine Nachricht erhalten hat, akzeptiert er diese (ACK) oder nicht (NACK, siehe 3. Redelivery). Ein Subscriber kann sich regulär abmelden (UNSUBSCRIBE). Ein Subscriber kann die Verbindung zum Message Broker verlieren (Netzwerkausfall), sich später wieder verbinden und wieder wiedererkannt (siehe 4. Durability).
3. Redelivery: Der Message Broker stellt sicher, dass sämtliche Subscriber von einem Topic die Nachricht erhalten. Wenn ein Subscriber die Nachricht ablehnt (NACK), wird sie nach einer bestimmten Zeit noch einmal versendet.
4. Durability: Der Message Broker stellt sicher, dass Subscriber, die die Verbindung verloren haben, sämtliche Nachrichten zugetellt bekommen, sobald sie sich wieder melden.
5. Managemnt Interface: (eventuell) Der Message Broker bietet ein Management Interface zur Abfrage von Statistiken (z.B. Anzahl Topics, Namen der Topics, Anzahl Subscriber/Publisher je Topic, Anzahl Nachrichten) an.

Teil von Stomp, aber explizit nicht Teil von diesem Projekt:
1. Queues
2. Transkationen
3. Login

1. Nachrichten, die von allen Subscribern empfangen wurden sollen aufgeräumt werden, gleichzeitig könnte sich jedoch noch ein weitere Subscriber für einen Topic anmelden
2. Zwei Publisher könnten zur gleichen Zeit eine Nachricht an den selben Topic senden

Memory Layout:
- Es gibt eine Speicherregion, wo alle Nachrichten pro Topic abgelegt werden. Wenn dann eine neue Nachricht eintrifft, wird diese dort abgelegt. Da es mehrere Publisher geben kann, muss dieser Bereich geschützt werden.

Herausforderungen im Bezug auf Concurrency (nicht vollständig, eher spontan):
Es gibt vier Aktoren auf dem Server/Broker:
    1. Socket Listener: Wartet auf Verbindungen von einem Publisher oder Subscriber und erstellt einen neuen Worker Prozess zum abhandlen des Requests.
    2. Worker: Interpretiert die Kommandos vom Publisher oder Subscriber und erstellt Topics (falls nötig), fügt Nachrichten ein und registriert Subscriber.
        a. Concurrency: Zwei Worker wollen gleichzeitig den gleichen Topic erstellen --> Speicherbereich muss in einem konsistenten Zustand gehalten werden
        b. Concurrency: Es gibt eine globale Liste von Subscribern die von Workern gefüllt werden und vom Garbage Collector abgeräumt wird. --> Speicherbereich muss synchronisiert werden da 1-n Prozesse schreibend drauf zugreifen (Worker) und der Garbage Collector lesend drauf zugreift.
    3. Distributor: Verteilt die Nachrichten an die Subscriber. Für jede Nachricht werden Statisktiken gehalten, um zu bestimmen, an welche Subscriber die Nachricht bereits zugestellt wurde und falls nicht, wieviel mal es fehlgeschlagen hat und um welche Zeit der letzte Versuch war.
        a. Concurrency: Der Speicherbereich, indem die Statisktiken gehalten werden, wird auch vom Garbage Collector (GC) verwendet, um zu entscheiden, ob die Nachricht abgeräumt werden kann. Der Distributor weiss nicht, ob er noch einmal versuchen soll, die Nachricht zuzustellen oder ob sie gerade vom GC abgeräumt werden soll. Der Statisktik-Bereich muss konsistent gehalten werden, sodass der GC korrekt entscheiden kann, ob die Nachricht abgeräumt werden kann. --> Synchronisation notwendig.
    4. Garbage Collector (GC): Prüft periodisch die Nachrichten in jeder Queue. Falls eine Nachricht an alle Subscriber erfolgreich zugestellt werden konnte, wird diese gelöscht. Wenn eine Nachricht schon genügend oft erfolglos zugestellt wurde, wird der Subscriber, an den die Nachricht nicht zugestellt werden konnte, gelöscht.
        a. Concurrency: Der GC bestimmt, ob eine Nachricht abgeräumt werden kann (siehe 3a).
        b. Concurrency: Der GC bestimmt, ob ein Subscriber abgeräumt wird (siehe 2b).
