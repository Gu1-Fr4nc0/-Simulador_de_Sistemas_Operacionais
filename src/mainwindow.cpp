/*
 * mainwindow.cpp
 * Interface gráfica Qt do Simulador de SO.
 * Compilado como C++ mas interage com o código em C usando 'extern "C"'.
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
/* Widget Personalizado para Desenhar o Gráfico de Gantt (Timeline)   */
/* ------------------------------------------------------------------ */
class WidgetLinhaDoTempo : public QWidget {
    Q_OBJECT;
public:
    WidgetLinhaDoTempo(QWidget *pai = nullptr) : QWidget(pai) {
        setMinimumHeight(120);
        setMinimumWidth(600);
    }

    /* Recebe os dados simulados para poder desenhar na tela */
    void setDados(const ResultadoEscalonador *r, int num_processos) {
        resultado = r;
        total_processos = num_processos;
        update(); /* Força o Qt a chamar o paintEvent para repintar a tela */
    }

protected:
    /* Evento chamado automaticamente pelo Qt sempre que a tela precisa ser desenhada */
    void paintEvent(QPaintEvent *) override {
        if (!resultado || resultado->quantidade_entradas == 0) return;

        QPainter pintor(this);
        pintor.setRenderHint(QPainter::Antialiasing); /* Suaviza as bordas do desenho */

        /* Paleta de cores estática para diferenciar os processos visualmente */
        static const QColor paleta[] = {
            QColor(52,152,219), QColor(46,204,113), QColor(231,76,60),
            QColor(155,89,182), QColor(230,126,34), QColor(26,188,156),
            QColor(241,196,15), QColor(149,165,166)
        };
        static const int NUM_CORES = 8;

        int tempo_total = resultado->entradas[resultado->quantidade_entradas - 1].tempo_fim;
        if (tempo_total == 0) return;

        /* Constantes de dimensionamento do desenho */
        const int ALTURA_LINHA = 40, MARGEM = 40, TOPO = 20;
        int LARGURA_UTIL = width() - 2 * MARGEM;

        /* Pinta o fundo do canvas de tema escuro */
        pintor.fillRect(rect(), QColor(20, 22, 34));

        /* Percorre todo o histórico de execuções na CPU para desenhar os blocos */
        for (int i = 0; i < resultado->quantidade_entradas; i++) {
            const EntradaLinhaDoTempo &entrada = resultado->entradas[i];
            
            /* Regra de 3 simples para saber a posição X na tela proporcional ao tempo */
            int x1 = MARGEM + (int)((double)entrada.tempo_inicio / tempo_total * LARGURA_UTIL);
            int x2 = MARGEM + (int)((double)entrada.tempo_fim   / tempo_total * LARGURA_UTIL);
            int largura_bloco  = x2 - x1;
            
            if (largura_bloco < 1) largura_bloco = 1;

            /* Se PID for -1 (ocioso), usa roxo/cinza escuro. Senão, pega uma cor da paleta */
            QColor cor = (entrada.id_processo == -1) ? QColor(43, 47, 76)
                                                     : paleta[entrada.id_processo % NUM_CORES];

            pintor.fillRect(x1, TOPO, largura_bloco, ALTURA_LINHA, cor);
            pintor.setPen(QPen(QColor(0, 240, 255), 1)); /* borda azul claro neon */
            pintor.drawRect(x1, TOPO, largura_bloco, ALTURA_LINHA);

            /* Escreve o texto dentro do bloco se houver espaço suficiente */
            if (largura_bloco > 20) {
                pintor.setPen(Qt::white);
                QString rotulo = (entrada.id_processo == -1) ? "ocioso"
                                                             : QString("P%1").arg(entrada.id_processo);
                pintor.drawText(x1, TOPO, largura_bloco, ALTURA_LINHA,
                           Qt::AlignCenter, rotulo);
            }
        }

        /* Desenha a régua de tempo na parte inferior */
        pintor.setPen(QColor(168, 178, 193));
        for (int i = 0; i <= tempo_total; i++) {
            int x = MARGEM + (int)((double)i / tempo_total * LARGURA_UTIL);
            pintor.drawLine(x, TOPO + ALTURA_LINHA, x, TOPO + ALTURA_LINHA + 8);
            pintor.drawText(x - 10, TOPO + ALTURA_LINHA + 10, 20, 15,
                       Qt::AlignCenter, QString::number(i));
        }
    }

private:
    const ResultadoEscalonador *resultado = nullptr;
    int total_processos = 0;
};

/* ------------------------------------------------------------------ */
/* Janela Principal da Aplicação                                      */
/* ------------------------------------------------------------------ */
class JanelaPrincipal : public QMainWindow {
    Q_OBJECT;
public:
    JanelaPrincipal(QWidget *pai = nullptr) : QMainWindow(pai) {
        setWindowTitle("Simulador de Sistemas Operacionais - UTFPR");
        setMinimumSize(900, 700);
        construirInterface();
    }

private slots:
    /* Chamado quando o usuário clica no botão de carregar CSV */
    void aoCarregarCSV() {
        QString caminho = QFileDialog::getOpenFileName(
            this, "Abrir CSV de Processos", "", "CSV (*.csv);;Todos (*.*)");
        
        if (caminho.isEmpty()) return;
        
        caminhoArquivoCSV = caminho;
        rotuloArquivo->setText(QFileInfo(caminho).fileName());

        /* Usa nossa função em C para ler o arquivo do disco */
        total_processos = carregar_processos_do_csv(caminho.toStdString().c_str(),
                                                    processos, MAX_PROCESSOS);
                                                    
        if (total_processos <= 0) {
            QMessageBox::critical(this, "Erro", "Falha ao carregar arquivo CSV ou o arquivo está vazio.");
            total_processos = 0;
            return;
        }
        
        preencherTabelaProcessos();
        statusBar()->showMessage(
            QString("Carregados %1 processos com sucesso.").arg(total_processos));
    }

    /* Chamado quando o usuário clica no botão Simular */
    void aoSimular() {
        if (total_processos <= 0) {
            QMessageBox::warning(this, "Aviso",
                "Por favor, carregue um arquivo CSV primeiro.");
            return;
        }

        /* 1. SIMULAÇÃO DO ESCALONADOR DE CPU */
        TipoEscalonador tipo_algoritmo = (TipoEscalonador)comboEscalonador->currentIndex();
        int quantum = spinBoxQuantum->value();
        
        executar_escalonador(processos, total_processos, tipo_algoritmo, quantum, &resultadoEscalonador);

        /* 2. SIMULAÇÃO DA GERÊNCIA DE MEMÓRIA */
        int mem_fisica_mb = spinBoxMemFisica->value();
        int mem_virtual_mb = spinBoxMemVirtual->value();
        PoliticaSubstituicaoPagina politica_memoria = (PoliticaSubstituicaoPagina)comboPoliticaMemoria->currentIndex();

        inicializar_memoria(&memoriaFisica, mem_fisica_mb, mem_virtual_mb);
        resultadoMemoria.total_falhas_pagina = 0;
        resultadoMemoria.total_acessos       = 0;

        TabelaPaginas tabela_paginas;
        for (int i = 0; i < total_processos; i++) {
            memset(&tabela_paginas, 0, sizeof(tabela_paginas));
            
            /* Tenta alocar a memória necessária para rodar este processo */
            int falhas = carregar_processo_memoria(&memoriaFisica, &tabela_paginas,
                                                   processos[i].id_processo,
                                                   processos[i].memoria_necessaria,
                                                   politica_memoria, processos[i].tempo_chegada,
                                                   processos, total_processos, nullptr);
                                                   
            resultadoMemoria.total_falhas_pagina += falhas;
            resultadoMemoria.total_acessos       += ((processos[i].memoria_necessaria * 1024) / TAMANHO_PAGINA_KB) * 3;
            
            /* Simula a liberação da memória pois o processo finalizou sua execução neste bloco */
            liberar_processo_memoria(&memoriaFisica, &tabela_paginas, processos[i].id_processo);
        }

        /* 3. ATUALIZA A INTERFACE VISUAL COM OS RESULTADOS */
        widgetLinhaDoTempo->setDados(&resultadoEscalonador, total_processos);
        
        rotuloMediaEspera->setText(
            QString("Espera Média: %1").arg(resultadoEscalonador.media_tempo_espera, 0, 'f', 2));
        rotuloMediaResposta->setText(
            QString("Resposta Média: %1").arg(resultadoEscalonador.media_tempo_resposta, 0, 'f', 2));
        rotuloFalhasPagina->setText(
            QString("Faltas de Página (Page Faults): %1").arg(resultadoMemoria.total_falhas_pagina));

        atualizarMetricasNaTabela();
        statusBar()->showMessage("Simulação concluída com sucesso.");
    }

private:
    /* --- Dados da Simulação --- */
    Processo             processos[MAX_PROCESSOS];
    int                  total_processos = 0;
    QString              caminhoArquivoCSV;
    ResultadoEscalonador resultadoEscalonador{};
    MemoriaFisica        memoriaFisica{};
    ResultadoMemoria     resultadoMemoria{};

    /* --- Elementos da Tela (Widgets) --- */
    QLabel             *rotuloArquivo         = nullptr;
    QComboBox          *comboEscalonador      = nullptr;
    QSpinBox           *spinBoxQuantum        = nullptr;
    QComboBox          *comboPoliticaMemoria  = nullptr;
    QSpinBox           *spinBoxMemFisica      = nullptr;
    QSpinBox           *spinBoxMemVirtual     = nullptr;
    WidgetLinhaDoTempo *widgetLinhaDoTempo    = nullptr;
    QLabel             *rotuloMediaEspera     = nullptr;
    QLabel             *rotuloMediaResposta   = nullptr;
    QLabel             *rotuloFalhasPagina    = nullptr;
    QTableWidget       *tabelaProcessos       = nullptr;

    /* Configura o layout e todos os componentes visuais da tela */
    void construirInterface() {
        QWidget *widgetCentral = new QWidget(this);
        setCentralWidget(widgetCentral);
        QVBoxLayout *layoutPrincipal = new QVBoxLayout(widgetCentral);

        /* --- Grupo 1: Configuração da Simulação --- */
        QGroupBox *grupoConfiguracao = new QGroupBox("Configurações do Sistema");
        QGridLayout *gridConfiguracao = new QGridLayout(grupoConfiguracao);

        QPushButton *botaoCarregar = new QPushButton("📂 Carregar CSV");
        rotuloArquivo = new QLabel("(nenhum arquivo carregado)");
        gridConfiguracao->addWidget(botaoCarregar, 0, 0);
        gridConfiguracao->addWidget(rotuloArquivo, 0, 1, 1, 3);

        gridConfiguracao->addWidget(new QLabel("Escalonador:"), 1, 0);
        comboEscalonador = new QComboBox();
        comboEscalonador->addItems({"Round-Robin (Circular)", "SJF Preemptivo (SRTF)", "Prioridade Preemptiva"});
        gridConfiguracao->addWidget(comboEscalonador, 1, 1);

        gridConfiguracao->addWidget(new QLabel("Fatia de Tempo (Quantum):"), 1, 2);
        spinBoxQuantum = new QSpinBox();
        spinBoxQuantum->setRange(1, 100);
        spinBoxQuantum->setValue(4);
        gridConfiguracao->addWidget(spinBoxQuantum, 1, 3);

        gridConfiguracao->addWidget(new QLabel("Substituição de Página:"), 2, 0);
        comboPoliticaMemoria = new QComboBox();
        comboPoliticaMemoria->addItems({"FIFO (Primeiro a Entrar)", "LRU (Menos Recente)", "Ótima"});
        gridConfiguracao->addWidget(comboPoliticaMemoria, 2, 1);

        gridConfiguracao->addWidget(new QLabel("Memória RAM Física (MB):"), 2, 2);
        spinBoxMemFisica = new QSpinBox();
        spinBoxMemFisica->setRange(64, 65536);
        spinBoxMemFisica->setValue(256);
        gridConfiguracao->addWidget(spinBoxMemFisica, 2, 3);

        gridConfiguracao->addWidget(new QLabel("Memória Virtual em Disco (MB):"), 3, 2);
        spinBoxMemVirtual = new QSpinBox();
        spinBoxMemVirtual->setRange(256, 131072);
        spinBoxMemVirtual->setValue(1024);
        gridConfiguracao->addWidget(spinBoxMemVirtual, 3, 3);

        QPushButton *botaoSimular = new QPushButton("▶  Iniciar Simulação");
        botaoSimular->setObjectName("botaoSimular");
        gridConfiguracao->addWidget(botaoSimular, 4, 0, 1, 4, Qt::AlignLeft);

        layoutPrincipal->addWidget(grupoConfiguracao);

        /* --- Grupo 2: Visualização da Linha do Tempo (Gantt) --- */
        QGroupBox *grupoLinhaTempo = new QGroupBox("Gráfico de Execução (Linha do Tempo)");
        QVBoxLayout *layoutLinhaTempo  = new QVBoxLayout(grupoLinhaTempo);
        
        widgetLinhaDoTempo = new WidgetLinhaDoTempo();
        QScrollArea *areaRolagem = new QScrollArea();
        areaRolagem->setWidget(widgetLinhaDoTempo);
        areaRolagem->setWidgetResizable(true);
        areaRolagem->setMinimumHeight(140);
        
        layoutLinhaTempo->addWidget(areaRolagem);
        layoutPrincipal->addWidget(grupoLinhaTempo);

        /* --- Grupo 3: Indicadores e Métricas --- */
        QGroupBox *grupoMetricas = new QGroupBox("Métricas Globais de Desempenho");
        QHBoxLayout *layoutMetricas  = new QHBoxLayout(grupoMetricas);
        
        rotuloMediaEspera   = new QLabel("Espera Média: --");
        rotuloMediaResposta = new QLabel("Resposta Média: --");
        rotuloFalhasPagina  = new QLabel("Faltas de Página: --");
        
        layoutMetricas->addWidget(rotuloMediaEspera);
        layoutMetricas->addWidget(rotuloMediaResposta);
        layoutMetricas->addWidget(rotuloFalhasPagina);
        layoutPrincipal->addWidget(grupoMetricas);

        /* --- Grupo 4: Lista de Processos --- */
        QGroupBox *grupoTabela = new QGroupBox("Detalhes dos Processos");
        QVBoxLayout *layoutTabela = new QVBoxLayout(grupoTabela);
        
        tabelaProcessos = new QTableWidget(0, 8);
        tabelaProcessos->setHorizontalHeaderLabels({
            "PID", "Nome", "Chegada", "Execução", "Prioridade", "Memória (MB)",
            "Espera", "Resposta"
        });
        tabelaProcessos->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        tabelaProcessos->setEditTriggers(QAbstractItemView::NoEditTriggers); /* Apenas leitura */
        
        layoutTabela->addWidget(tabelaProcessos);
        layoutPrincipal->addWidget(grupoTabela);

        /* --- Conexões de Eventos (Sinais e Slots) --- */
        connect(botaoCarregar, &QPushButton::clicked, this, &JanelaPrincipal::aoCarregarCSV);
        connect(botaoSimular,  &QPushButton::clicked, this, &JanelaPrincipal::aoSimular);
        
        /* Desabilita o campo de Quantum se o algoritmo escolhido não for Round-Robin */
        connect(comboEscalonador, QOverload<int>::of(&QComboBox::currentIndexChanged),
                [this](int indice_selecionado){ spinBoxQuantum->setEnabled(indice_selecionado == 0); });
    }

    /* Lê os dados em memória e popula as linhas e colunas da tabela visual */
    void preencherTabelaProcessos() {
        tabelaProcessos->setRowCount(total_processos);
        for (int i = 0; i < total_processos; i++) {
            const Processo &p = processos[i];
            tabelaProcessos->setItem(i, 0, new QTableWidgetItem(QString::number(p.id_processo)));
            tabelaProcessos->setItem(i, 1, new QTableWidgetItem(p.nome));
            tabelaProcessos->setItem(i, 2, new QTableWidgetItem(QString::number(p.tempo_chegada)));
            tabelaProcessos->setItem(i, 3, new QTableWidgetItem(QString::number(p.tempo_execucao)));
            tabelaProcessos->setItem(i, 4, new QTableWidgetItem(QString::number(p.prioridade)));
            tabelaProcessos->setItem(i, 5, new QTableWidgetItem(QString::number(p.memoria_necessaria)));
            tabelaProcessos->setItem(i, 6, new QTableWidgetItem("--")); /* Espera (ainda não calculada) */
            tabelaProcessos->setItem(i, 7, new QTableWidgetItem("--")); /* Resposta (ainda não calculada) */
        }
    }

    /* Após simular, preenche os valores calculados nas duas últimas colunas */
    void atualizarMetricasNaTabela() {
        for (int i = 0; i < total_processos; i++) {
            const Processo &p = processos[i];
            tabelaProcessos->setItem(i, 6, new QTableWidgetItem(QString::number(p.tempo_espera)));
            tabelaProcessos->setItem(i, 7, new QTableWidgetItem(QString::number(p.tempo_resposta)));
        }
    }
};

/* 
 * Um requisito técnico do Qt para permitir que as macros Q_OBJECT funcionem 
 * se declaradas no próprio arquivo .cpp 
 */
#include "mainwindow.moc"

/* ------------------------------------------------------------------ */
/* Função Exportada para o C (Usada pela main.c)                      */
/* ------------------------------------------------------------------ */
extern "C" int executar_interface_grafica(int argc, char *argv[]) {
    QApplication aplicacao(argc, argv);
    aplicacao.setStyle("Fusion"); /* Deixa com aparência moderna (multiplataforma) */
    
    QString estiloTech = R"(
        QWidget {
            background-color: #0d0e15;
            color: #e0e6ed;
            font-family: 'Segoe UI', Arial, sans-serif;
            font-size: 13px;
        }
        QMainWindow {
            background-color: #0d0e15;
        }
        QGroupBox {
            border: 2px solid #6320ee; /* Roxo tech */
            border-radius: 12px;
            margin-top: 18px;
            padding: 12px;
            background-color: #141622;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            padding: 0 8px;
            color: #00f0ff; /* Ciano / Azul neon */
            font-weight: bold;
            font-size: 14px;
        }
        QPushButton {
            background-color: #6320ee;
            color: white;
            border: none;
            border-radius: 8px;
            padding: 8px 16px;
            font-weight: bold;
            transition: background-color 0.3s;
        }
        QPushButton:hover {
            background-color: #8048ff;
            border: 1px solid #00f0ff;
        }
        QPushButton:pressed {
            background-color: #00f0ff;
            color: #0d0e15;
            padding-top: 10px;
            padding-bottom: 6px;
        }
        QPushButton#botaoSimular {
            background-color: #00f0ff;
            color: #0d0e15;
            font-size: 15px;
            border-radius: 10px;
            padding: 10px 20px;
        }
        QPushButton#botaoSimular:hover {
            background-color: #6320ee;
            color: white;
            border: 2px solid #00f0ff;
        }
        QPushButton#botaoSimular:pressed {
            background-color: #8048ff;
            padding-top: 12px;
            padding-bottom: 8px;
        }
        QComboBox, QSpinBox, QLineEdit {
            background-color: #1a1d2d;
            color: #e0e6ed;
            border: 1px solid #6320ee;
            border-radius: 6px;
            padding: 5px;
        }
        QComboBox:hover, QSpinBox:hover {
            border: 1px solid #00f0ff;
        }
        QComboBox::drop-down {
            border: none;
        }
        QTableWidget {
            background-color: #141622;
            color: #e0e6ed;
            gridline-color: #2b2f4c;
            border: 2px solid #6320ee;
            border-radius: 10px;
            outline: none;
        }
        QHeaderView::section {
            background-color: #1a1d2d;
            color: #00f0ff;
            padding: 6px;
            border: 1px solid #2b2f4c;
            font-weight: bold;
        }
        QTableWidget::item:selected {
            background-color: #6320ee;
            color: white;
        }
        QScrollArea {
            border: none;
            background-color: transparent;
        }
        QScrollBar:vertical {
            background: #0d0e15;
            width: 12px;
            margin: 0px 0px 0px 0px;
        }
        QScrollBar::handle:vertical {
            background: #6320ee;
            min-height: 20px;
            border-radius: 6px;
        }
        QScrollBar::handle:vertical:hover {
            background: #00f0ff;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
        QScrollBar:horizontal {
            background: #0d0e15;
            height: 12px;
            margin: 0px 0px 0px 0px;
        }
        QScrollBar::handle:horizontal {
            background: #6320ee;
            min-width: 20px;
            border-radius: 6px;
        }
        QScrollBar::handle:horizontal:hover {
            background: #00f0ff;
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0px;
        }
        QStatusBar {
            color: #00f0ff;
            font-weight: bold;
        }
        QLabel {
            color: #a8b2c1;
        }
    )";
    aplicacao.setStyleSheet(estiloTech);
    
    JanelaPrincipal janela;
    janela.show();   
    
    return aplicacao.exec(); /* Inicia o loop de eventos da interface gráfica */
}
