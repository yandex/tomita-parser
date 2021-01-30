package com.srg.tomita;

import com.srg.tomita.helper.TomitaTestHelper;
import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

import java.util.List;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Future;
import java.util.stream.Collectors;
import java.util.stream.IntStream;

public class TestPooledParser {

    private TomitaTestHelper helper;

    @Before
    public void init() {
        helper = new TomitaTestHelper(this.getClass().getSimpleName());
    }

    @After
    public void dispose() {
        helper.dispose();
    }

    @Test
    public void testParser() throws ExecutionException, InterruptedException {
        TomitaPooledParser parser = helper.createPooledParser(10);
        Future<String> result = parser.parse(helper.getDocuments());
        String response = result.get();
        helper.assertResponse(response);
    }

    @Test
    public void testParallel() {
        int requestCount = 20;

        TomitaPooledParser parser = helper.createPooledParser(10);

        List<String> outputList = IntStream.range(0, requestCount)
                .mapToObj(i -> parser.parse(helper.getDocuments()))
                .collect(Collectors.toList())
                .stream()
                .map(f -> {
                    try {
                        return f.get();
                    } catch (InterruptedException e) {
                        throw new RuntimeException("The test was interrupted");
                    } catch (ExecutionException e) {
                        throw new RuntimeException("Exception occurred", e.getCause());
                    }
                })
                .collect(Collectors.toList());

        Assert.assertEquals(requestCount, outputList.size());
        outputList.forEach(helper::assertResponse);
    }

    @Test
    public void seriousTest() {
        // Длительный "нагрузочный" тест. Тестирует в основном утечку в нативном коде.
        // Избегаем аккумуляции данных на яве, чтобы избежать потребления памяти ява-объектами
        //int requestCount = 10000;
        int requestCount = 100; // более скромное значение, чтобы сильно не замедлять сборку проектов

        TomitaPooledParser parser = helper.createPooledParser(3);

        IntStream.range(0, requestCount)
                .mapToObj(i -> parser.parse(helper.getDocuments()))
                .collect(Collectors.toList())
                .forEach(f -> {
                    String response;
                    try {
                        response = f.get();
                    } catch (InterruptedException e) {
                        throw new RuntimeException("The test was interrupted");
                    } catch (ExecutionException e) {
                        throw new RuntimeException("Exception occurred", e.getCause());
                    }

                    helper.assertResponse(response);
                    helper.setTotalProcessedBytes(helper.getDocumentsSize());
                    helper.logSpeed();
                });
    }
}
