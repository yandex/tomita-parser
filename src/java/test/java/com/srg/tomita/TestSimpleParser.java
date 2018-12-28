package com.srg.tomita;

import com.srg.tomita.helper.TomitaTestHelper;
import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

public class TestSimpleParser {
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
    public void testParser() {
        TomitaParser parser = helper.createParser();
        String response = parser.parse(helper.getDocuments());
        System.out.println(response);
        // Проверяем, что вернулся какой-то xml
        Assert.assertTrue(response.startsWith("<?xml version='1.0' encoding='utf-8'?>"));
    }
}
