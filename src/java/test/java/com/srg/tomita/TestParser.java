package com.srg.tomita;

import com.srg.tomita.helper.TomitaTestHelper;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

public class TestParser {

    TomitaTestHelper helper;

    @Before
    public void init() {
        helper = new TomitaTestHelper(this.getClass().getSimpleName());
    }

    @After
    public void dispose() {
        helper.dispose();
    }

    @Test
    public void testParser() {
        TomitaParser parser = helper.createParser();
        String response = parser.parse(helper.getDocuments());
        System.out.println(response);
        helper.assertResponse(response);
    }

    /**
     * Важный тест, который проверяет одинаковость первого
     * и последующего парсинга (поскольку избегаем динамического
     * выделения памяти в нативном коде при/перед парсингом и объекты
     * не пересоздаются, важно проверить, что состояние парсера и оберток
     * после парсинга скидывается к начальному)
     */
    @Test
    public void testSubsequentParserCall() {
        TomitaParser parser = helper.createParser();
        String response1 = parser.parse(helper.getDocuments());
        System.out.println(response1);
        helper.assertResponse(response1);

        String response2 = parser.parse(helper.getDocuments());
        System.out.println(response2);
        helper.assertResponse(response2);
    }
}
