# CMPT300_Assignment2

This repo is for CMPT 300 – Operating Systems - HW2: Bounded Buffer Problem

In this assignment, there are 3 generators, and each produces a unique kind of material independently. All materials are stored in an input buffer with size 10 before they are forwarded to the operators. The 3 operators with same priority which are responsible for producing the products based on the materials. Each product needs 2 different kinds of materials. Each time an operator need 2 tools. There are 3 tools provided for operators. An operator can process one product at one time. When an operator gets both the materials and tools, it can produce a product within a limited time varied from 0.01 second to 1 second. Otherwise, it wait until all the necessities are met. It can grab the materials or tools first..

