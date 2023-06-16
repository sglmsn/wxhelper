package com.example.wxhk;

import org.dromara.hutool.core.lang.Console;
import org.junit.jupiter.api.Test;

public class MathTest {


    @Test
    void t1() {


        /**
         * 1. 方程式   A*13+B*12+C*3=412,500
         * 2. C大于等于B, B大于等于A
         * 3. A, B, C区间在10,000 - 20,000
         * 4. A, B, C是500的整数
         */
        int sum = 412500;
        for (int a = 10000; a <= 20000; a += 500) {
            for (int b = a; b <= 20000; b += 500) {
                for (int c = b; c <= 20000; c += 500) {
                    if (a * 13 + b * 12 + c * 3 == sum) {
                        Console.log(a, b, c);
                    }
                }
            }
        }
    }
}
