#for testing all rule application
aa {writeLine("stdout","doing Rule aa definition 1"); writeLine("stdout"," aa calls bb"); bb; writeLine("stdout"," aa calls cc"); cc; }
aa {writeLine("stdout","doing Rule aa definition 2"); }
aa {ON("1" == "2") {writeLine("stdout","doing Rule aa definition 3"); }}
aa {writeLine("stdout","doing Rule aa definition 4"); }
bb {writeLine("stdout","  doing Rule bb definition 1"); writeLine("stdout","   bb calls dd"); dd; }
bb {writeLine("stdout","  doing Rule bb definition 2"); }
cc {writeLine("stdout","  doing Rule cc definition 1"); }
cc {ON("1" == "2") {writeLine("stdout","  doing Rule cc definition 2"); }}
cc {writeLine("stdout","  doing Rule cc definition 3"); }
dd {writeLine("stdout","    doing Rule dd definition 1"); }
dd {writeLine("stdout","    doing Rule dd definition 2"); }
