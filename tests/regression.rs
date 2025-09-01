use assert_cmd::Command;

fn regression_test(name: &str, iterations: usize) -> Result<(), anyhow::Error> {
    let mut cmd = Command::cargo_bin(env!("CARGO_PKG_NAME"))?;
    cmd.args([
        "file",
        &format!("res/{name}.json"),
        &format!("{iterations}"),
        "-o",
        &format!("res/expected/expected_{name}.log"),
        "-t",
        &format!("out/timing/timing_{name}.log"),
    ]);
    cmd.assert().success();

    Ok(())
}

#[test]
fn sl002() -> anyhow::Result<()> {
    regression_test("sl002", 24)
}

#[test]
fn sl003() -> anyhow::Result<()> {
    regression_test("sl003", 1)
}

#[test]
fn sl005b() -> anyhow::Result<()> {
    regression_test("sl005b", 12)
}

#[test]
fn sl007() -> anyhow::Result<()> {
    regression_test("sl007", 100)
}

#[test]
fn sl012() -> anyhow::Result<()> {
    regression_test("sl012", 2000)
}
