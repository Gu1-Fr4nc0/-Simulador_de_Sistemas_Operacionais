/*
 * mainwindow.cpp
 * Interface gráfica Qt do Simulador de SO.
 * Compilado como C++ mas expõe interface C via extern "C".
 */

#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QFileDialog>
#include <QTableWidget>
#include <QHeaderView>
#include <QPainter>
#include <QScrollArea>
#include <QGroupBox>
#include <QMessageBox>
#include <QString>
#include <QColor>
#include <vector>
#include <map>
#include <QStatusBar>

extern "C" {
#include "../include/process.h"
#include "../include/scheduler.h"
#include "../include/memory.h"
}

/* ------------------------------------------------------------------ */
/* Widget de desenho da Linha do Tempo                                  */
/* ------------------------------------------------------------------ */
class TimelineWidget : public QWidget {
    Q_OBJECT;
public:
    TimelineWidget(QWidget *parent = nullptr) : QWidget(parent) {
        setMinimumHeight(120);
        setMinimumWidth(600);
    }

    void setData(const SchedulerResult *r, int n_procs) {
        result   = r;
        nProcs   = n_procs;
        update(); /* solicita repintura */
    }

protected:
    void paintEvent(QPaintEvent *) override {
        if (!result || result->count == 0) return;

        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        /* Paleta de cores para cada PID */
        static const QColor palette[] = {
            QColor(52,152,219), QColor(46,204,113), QColor(231,76,60),
            QColor(155,89,182), QColor(230,126,34), QColor(26,188,156),
            QColor(241,196,15), QColor(149,165,166)
        };
        static const int NCOLORS = 8;

        int total_time = result->entries[result->count - 1].time_end;
        if (total_time == 0) return;

        const int ROW_H = 40, MARGIN = 40, TOP = 20;
        int W = width() - 2 * MARGIN;

        /* Fundo */
        p.fillRect(rect(), QColor(245, 245, 245));

        for (int i = 0; i < result->count; i++) {
            const TimelineEntry &e = result->entries[i];
            int x1 = MARGIN + (int)((double)e.time_start / total_time * W);
            int x2 = MARGIN + (int)((double)e.time_end   / total_time * W);
            int w  = x2 - x1;
            if (w < 1) w = 1;

            QColor c = (e.pid == -1) ? QColor(200,200,200)
                                     : palette[e.pid % NCOLORS];

            p.fillRect(x1, TOP, w, ROW_H, c);
            p.setPen(Qt::black);
            p.drawRect(x1, TOP, w, ROW_H);

            if (w > 20) {
                p.setPen(Qt::white);
                QString lbl = (e.pid == -1) ? "idle"
                                            : QString("P%1").arg(e.pid);
                p.drawText(x1, TOP, w, ROW_H,
                           Qt::AlignCenter, lbl);
            }
        }

        /* Marcadores de tempo */
        p.setPen(Qt::black);
        for (int i = 0; i <= total_time; i++) {
            int x = MARGIN + (int)((double)i / total_time * W);
            p.drawLine(x, TOP + ROW_H, x, TOP + ROW_H + 8);
            p.drawText(x - 10, TOP + ROW_H + 10, 20, 15,
                       Qt::AlignCenter, QString::number(i));
        }
    }

private:
    const SchedulerResult *result = nullptr;
    int nProcs = 0;
};

/* ------------------------------------------------------------------ */
/* Janela Principal                                                      */
/* ------------------------------------------------------------------ */
class MainWindow : public QMainWindow {
    Q_OBJECT;
public:
    MainWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Simulador de Sistemas Operacionais - UTFPR");
        setMinimumSize(900, 700);
        buildUI();
    }

private slots:
    void onLoadCSV() {
        QString path = QFileDialog::getOpenFileName(
            this, "Abrir CSV de Processos", "", "CSV (*.csv);;Todos (*.*)");
        if (path.isEmpty()) return;
        csvPath = path;
        lblFile->setText(QFileInfo(path).fileName());

        nProcs = load_processes_from_csv(path.toStdString().c_str(),
                                         procs, MAX_PROCESSES);
        if (nProcs <= 0) {
            QMessageBox::critical(this, "Erro", "Falha ao carregar CSV.");
            nProcs = 0;
            return;
        }
        populateTable();
        statusBar()->showMessage(
            QString("Carregados %1 processos.").arg(nProcs));
    }

    void onSimulate() {
        if (nProcs <= 0) {
            QMessageBox::warning(this, "Aviso",
                "Carregue um arquivo CSV primeiro.");
            return;
        }

        /* Escalonamento */
        SchedulerType stype = (SchedulerType)cmbSched->currentIndex();
        int quantum = spinQuantum->value();
        run_scheduler(procs, nProcs, stype, quantum, &schedResult);

        /* Memória */
        int phys_mb = spinPhysMem->value();
        int virt_mb = spinVirtMem->value();
        PageReplacePolicy mpol = (PageReplacePolicy)cmbMemPolicy->currentIndex();

        memory_init(&physMem, phys_mb, virt_mb);
        memResult.total_page_faults = 0;
        memResult.total_accesses    = 0;

        PageTable pt;
        for (int i = 0; i < nProcs; i++) {
            memset(&pt, 0, sizeof(pt));
            int faults = memory_load_process(&physMem, &pt,
                                             procs[i].pid,
                                             procs[i].memory_needed,
                                             mpol, procs[i].arrival_time,
                                             procs, nProcs, nullptr);
            memResult.total_page_faults += faults;
            memResult.total_accesses    += ((procs[i].memory_needed * 1024) / PAGE_SIZE_KB) * 3;
            memory_free_process(&physMem, &pt, procs[i].pid);
            // for (int i = 0; i < nProcs; i++){
            //     memory_free_process(&physMem, &pt, procs[i].pid);
            // }
        }


        /* Atualiza UI */
        timeline->setData(&schedResult, nProcs);
        lblAvgWait->setText(
            QString("Espera Média: %1").arg(schedResult.avg_waiting_time, 0, 'f', 2));
        lblAvgResp->setText(
            QString("Resposta Média: %1").arg(schedResult.avg_response_time, 0, 'f', 2));
        lblPageFaults->setText(
            QString("Page Faults: %1").arg(memResult.total_page_faults));

        updateTableMetrics();
        statusBar()->showMessage("Simulação concluída.");
    }

private:
    /* Dados */
    Process         procs[MAX_PROCESSES];
    int             nProcs = 0;
    QString         csvPath;
    SchedulerResult schedResult{};
    PhysicalMemory  physMem{};
    MemoryResult    memResult{};

    /* Widgets */
    QLabel         *lblFile      = nullptr;
    QComboBox      *cmbSched     = nullptr;
    QSpinBox       *spinQuantum  = nullptr;
    QComboBox      *cmbMemPolicy = nullptr;
    QSpinBox       *spinPhysMem  = nullptr;
    QSpinBox       *spinVirtMem  = nullptr;
    TimelineWidget *timeline     = nullptr;
    QLabel         *lblAvgWait   = nullptr;
    QLabel         *lblAvgResp   = nullptr;
    QLabel         *lblPageFaults= nullptr;
    QTableWidget   *table        = nullptr;

    void buildUI() {
        QWidget *central = new QWidget(this);
        setCentralWidget(central);
        QVBoxLayout *mainLayout = new QVBoxLayout(central);

        /* --- Grupo de Configuração --- */
        QGroupBox *grpConfig = new QGroupBox("Configuração");
        QGridLayout *cfgGrid = new QGridLayout(grpConfig);

        QPushButton *btnLoad = new QPushButton("📂 Carregar CSV");
        lblFile = new QLabel("(nenhum arquivo)");
        cfgGrid->addWidget(btnLoad, 0, 0);
        cfgGrid->addWidget(lblFile, 0, 1, 1, 3);

        cfgGrid->addWidget(new QLabel("Algoritmo:"), 1, 0);
        cmbSched = new QComboBox();
        cmbSched->addItems({"Round-Robin", "SJF Preemptivo",
                             "Prioridade Preemptiva"});
        cfgGrid->addWidget(cmbSched, 1, 1);

        cfgGrid->addWidget(new QLabel("Quantum:"), 1, 2);
        spinQuantum = new QSpinBox();
        spinQuantum->setRange(1, 100);
        spinQuantum->setValue(4);
        cfgGrid->addWidget(spinQuantum, 1, 3);

        cfgGrid->addWidget(new QLabel("Substituição:"), 2, 0);
        cmbMemPolicy = new QComboBox();
        cmbMemPolicy->addItems({"FIFO", "LRU", "Ótimo"});
        cfgGrid->addWidget(cmbMemPolicy, 2, 1);

        cfgGrid->addWidget(new QLabel("Mem. Física (MB):"), 2, 2);
        spinPhysMem = new QSpinBox();
        spinPhysMem->setRange(64, 65536);
        spinPhysMem->setValue(256);
        cfgGrid->addWidget(spinPhysMem, 2, 3);

        cfgGrid->addWidget(new QLabel("Mem. Virtual (MB):"), 3, 2);
        spinVirtMem = new QSpinBox();
        spinVirtMem->setRange(256, 131072);
        spinVirtMem->setValue(1024);
        cfgGrid->addWidget(spinVirtMem, 3, 3);

        QPushButton *btnSim = new QPushButton("▶  Simular");
        btnSim->setStyleSheet("font-weight:bold; padding:6px;");
        cfgGrid->addWidget(btnSim, 4, 0, 1, 4, Qt::AlignLeft);

        mainLayout->addWidget(grpConfig);

        /* --- Linha do Tempo --- */
        QGroupBox *grpTimeline = new QGroupBox("Linha do Tempo");
        QVBoxLayout *tlLayout  = new QVBoxLayout(grpTimeline);
        timeline = new TimelineWidget();
        QScrollArea *sa = new QScrollArea();
        sa->setWidget(timeline);
        sa->setWidgetResizable(true);
        sa->setMinimumHeight(140);
        tlLayout->addWidget(sa);
        mainLayout->addWidget(grpTimeline);

        /* --- Métricas --- */
        QGroupBox *grpMetrics = new QGroupBox("Métricas");
        QHBoxLayout *mLayout  = new QHBoxLayout(grpMetrics);
        lblAvgWait    = new QLabel("Espera Média: --");
        lblAvgResp    = new QLabel("Resposta Média: --");
        lblPageFaults = new QLabel("Page Faults: --");
        mLayout->addWidget(lblAvgWait);
        mLayout->addWidget(lblAvgResp);
        mLayout->addWidget(lblPageFaults);
        mainLayout->addWidget(grpMetrics);

        /* --- Tabela de Processos --- */
        QGroupBox *grpTable = new QGroupBox("Processos");
        QVBoxLayout *tbLayout = new QVBoxLayout(grpTable);
        table = new QTableWidget(0, 8);
        table->setHorizontalHeaderLabels({
            "PID","Nome","Chegada","tempo_cpu","Prior.","Mem(MB)",
            "Espera","Resposta"
        });
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        tbLayout->addWidget(table);
        mainLayout->addWidget(grpTable);

        /* Conexões */
        connect(btnLoad, &QPushButton::clicked, this, &MainWindow::onLoadCSV);
        connect(btnSim,  &QPushButton::clicked, this, &MainWindow::onSimulate);
        connect(cmbSched, QOverload<int>::of(&QComboBox::currentIndexChanged),
                [this](int idx){ spinQuantum->setEnabled(idx == 0); });
    }

    void populateTable() {
        table->setRowCount(nProcs);
        for (int i = 0; i < nProcs; i++) {
            const Process &p = procs[i];
            table->setItem(i, 0, new QTableWidgetItem(QString::number(p.pid)));
            table->setItem(i, 1, new QTableWidgetItem(p.name));
            table->setItem(i, 2, new QTableWidgetItem(QString::number(p.arrival_time)));
            table->setItem(i, 3, new QTableWidgetItem(QString::number(p.burst_time)));
            table->setItem(i, 4, new QTableWidgetItem(QString::number(p.priority)));
            table->setItem(i, 5, new QTableWidgetItem(QString::number(p.memory_needed)));
            table->setItem(i, 6, new QTableWidgetItem("--"));
            table->setItem(i, 7, new QTableWidgetItem("--"));
        }
    }

    void updateTableMetrics() {
        for (int i = 0; i < nProcs; i++) {
            const Process &p = procs[i];
            table->setItem(i, 6, new QTableWidgetItem(
                QString::number(p.waiting_time)));
            table->setItem(i, 7, new QTableWidgetItem(
                QString::number(p.response_time)));
        }
    }
};

#include "mainwindow.moc"

/* ------------------------------------------------------------------ */
/* Ponto de entrada C                                                    */
/* ------------------------------------------------------------------ */
extern "C" int gui_run(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setStyle("Fusion");
    MainWindow w;
w.show();   
    return app.exec();
}
