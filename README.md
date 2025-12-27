# quickjsflow
this is a repo created under ai control-flow 

首先开一个 Agent 窗口，这个 Agent 的角色是产品经理或者架构师，负责和我聊需求与架构设计，拆分任务，最后转换成可执行的需求说明，直接写到 github issue。如果功能比较复杂，就拆分成多个子 issue。注意，这个 Agent 不做具体的任务，保证它的上下文不会很快被填满，让它持续拥有全局视角。

然后，启动一个 Coder Agent，丢一个 github issue 进去，要求完成代码并提交 PR。权限给够，让他不要中途申请权限或者再来咨询。

提交 PR 后让另外一个 Reviewer Agent review  PR，或者 Github Copilot 也可以。有了 review 结果后，丢 PR 链接给 Coder Agent，让他继续修复。

最后没问题合并代码，继续下一个循环。是否能并行取决于 issue 之间是否有依赖关系。
