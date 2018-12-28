package com.srg.tomita;

import java.io.File;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * Многопоточный вариант {@link TomitaParser}.
 * <p>
 * Использует для парсинга пул объектов {@link TomitaParser}, одинаковым образом сконфигурированных.
 * Для парсинга будет использован первый свободный парсер. Поскольку кол-во создаваемых
 * парсеров равно кол-ву потоков в пуле, то всегда будет как минимум  один доступный парсер для задачи.
 */
public class TomitaPooledParser {

    private static final String PARSER_THREAD_PREFIX = "PARSER-THREAD";

    private static final int TERMINATION_TIMEOUT_SEC = 80;
    private static final int PARSER_WAIT_TIMEOUT_SEC = 120;

    private final int nThreads;

    private final List<TomitaParser> parsers;
    /*
     * Чтобы не зависеть от реализации тредпула, не полагаемся на thread local потоков внутри пула и
     * содержим свою очередь парсеров.
     */
    private final BlockingQueue<TomitaParser> parserQueue;

    private final ExecutorService executor;

    private static ThreadFactory PARSER_THREAD_FACTORY = new ThreadFactory() {
        private final AtomicInteger number = new AtomicInteger(1);

        @Override
        public Thread newThread(Runnable runnable) {
            return new Thread(runnable, PARSER_THREAD_PREFIX + "-" + number.getAndIncrement());
        }
    };

    public TomitaPooledParser(int nThreads, File confDir, String[] argv) {

        if (nThreads <= 0) {
            throw new IllegalArgumentException("The positive number of threads is expected");
        }

        this.nThreads = nThreads;

        this.parsers = new ArrayList<>(nThreads);
        this.parserQueue = new LinkedBlockingQueue<>(nThreads);

        for (int i = 0; i < nThreads; i++) {
            TomitaParser parser = new TomitaParser(confDir, argv);
            this.parsers.add(parser);
            this.parserQueue.add(parser);
        }

        executor = Executors.newFixedThreadPool(nThreads, PARSER_THREAD_FACTORY);
    }

    public int getThreadCount() {
        return nThreads;
    }

    /**
     * Метод выполнит асинхронный парсинг предоставленных документов в первом свободном парсере.
     *
     * @param input входные документы, которые будут парсится
     * @return Future с результатами парсинга.
     */
    public Future<String> parse(Collection<String> input) {
        return executor.submit(() -> tryParse(input));
    }

    private String tryParse(Collection<String> input) throws InterruptedException {
        // Хотя при количестве парсеров == количеству потоков в тредпуле никогда не может
        // случиться ситуации, что нет свободных парсеров (так как каждый поток возвращает парсер
        // в очередь до своего завершения, желательно вызывать блокирующий метод с таймаутом
        TomitaParser parser = parserQueue.poll(PARSER_WAIT_TIMEOUT_SEC, TimeUnit.SECONDS);
        if (parser == null) { // Мы не получили свободный парсер в течении таймаута
            throw new IllegalStateException("Failed to obtain free parser");
        }

        String output;
        try {
            output = parser.parse(input);
        } finally {
            parserQueue.offer(parser);
        }

        return output;
    }

    public synchronized void dispose() throws InterruptedException {
        this.executor.shutdown();
        if (!this.executor.awaitTermination(TERMINATION_TIMEOUT_SEC, TimeUnit.SECONDS)) {
            throw new IllegalStateException(String.format(
                    "Failed to shutdown TomitaPooledParser's thread pool in %d sec",
                    TERMINATION_TIMEOUT_SEC));
        }

        for (TomitaParser parser : this.parsers) {
            parser.dispose();
        }
    }
}
