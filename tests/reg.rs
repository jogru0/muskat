use assert_cmd::Command;

#[test]
fn sl003() -> anyhow::Result<()> {
    let mut cmd = Command::cargo_bin(env!("CARGO_PKG_NAME"))?;
    cmd.args([
        "file",
        "res/sl003.json",
        "24",
        "-o",
        "res/expected/sl003.log",
    ]);
    cmd.assert().success();

    Ok(())
}
